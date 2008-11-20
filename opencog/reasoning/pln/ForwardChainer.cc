#include "ForwardChainer.h"
#include "AtomSpaceWrapper.h"
#include "utils/NMPrinter.h"

#include <opencog/util/Logger.h>
#include <opencog/util/mt19937ar.h>

#include <boost/variant.hpp>
#include <time.h>

namespace haxx {
    extern reasoning::iAtomSpaceWrapper *defaultAtomSpaceWrapper;
}

namespace reasoning {

ForwardChainer::ForwardChainer()
{
    float minConfidence = FWD_CHAIN_MIN_CONFIDENCE;
    float probStack = FWD_CHAIN_PROB_STACK; 
    float probGlobal = FWD_CHAIN_PROB_GLOBAL;
}

ForwardChainer::~ForwardChainer()
{
}

int ForwardChainer::fillStack(int number, bool random)
{
    std::set<Handle> newSeeds;
    // create seed stack (using random or based on attention)
    if (random) {
        while (number > 0) {
            newSeeds.insert(GET_ATW->getRandomHandle(ATOM));
            number--;
        }
    } else {
        // use atomspace to get top STI atoms
        HandleSeq hs = GET_ATW->getImportantHandles(number);
        copy(hs.begin(), hs.end(), back_inserter(seedStack));
    }
    // remove duplicates from stack
    copy(newSeeds.begin(), newSeeds.end(), back_inserter(seedStack));
    return number - newSeeds.size();
}

Handle ForwardChainer::fwdChainToTarget(Handle target, int maxRuleApps)
{
    // TODO: Do some intelligent stuff here until the target is reached.
    // although maybe not... isn't this just what backward chaining does?
    return NULL;
}

// Static/Shared random number generator
RandGen* ForwardChainer::rng = NULL;

RandGen* ForwardChainer::getRNG() {
    if (!rng)
        rng = new opencog::MT19937RandGen((unsigned long) time(NULL));
    return rng;
}

Handle ForwardChainer::getRandomArgument(const std::vector< Vertex > &args)
{
    Handle a = NULL;
    int counter = 0;
    const int maxTries = 50; // Should only be a problem with very small atomSpaces
                        // but guard against it.
    cout << "FWDCHAIN Getting random handle" << endl;
    while ((!a) && counter < maxTries) {
        if (getRNG()->randfloat() <= probStack) {
            a = seedStack[(int) getRNG()->randfloat() * seedStack.size()];
        } else {
            a = GET_ATW->getRandomHandle(ATOM);
        }
        // Check if a is already in args
        foreach(Vertex v, args) {
            if (boost::get<Handle>(v) == a) {
                a = NULL;
                break;
            }
        }
        counter++;
    }
    if (counter == maxTries) {
        opencog::logger().error("Can't find a random argument that isn't in argument list already.");
    }
    return a;
}

HandleSeq ForwardChainer::getLocalLink(Handle lh, const std::vector< Vertex > &args) {
    AtomSpaceWrapper *atw = GET_ATW;
    HandleSeq choices;
    cout << "get local link" <<endl;
    // foreach outgoing dest (currently only supports target of lh)
    // check link is the source in the args... since this is a hack and Rules
    // don't check ordering when validating.
    if ( lh != v2h(args[0]) ) {
        cout << "error: lh is not the source." <<endl;
        return choices;
    }
    // if lh is a link:
    if (atw->isSubType(lh,LINK)) {
        cout << "lh is a link" <<endl;
        Handle junction = atw->getOutgoing(lh,1);
        // Only one outgoing if the link is asymmetric?
        HandleSeq inhs = atw->getIncoming(junction);
        // foreach incoming
        foreach (Handle inh, inhs) {
            if (inh != lh && atw->getOutgoing(inh,0) == junction) {
                // add to vector
                choices.push_back(inh);
            }
        }
    }
    return choices;
}

void ForwardChainer::printVertexVectorHandles(std::vector< Vertex > hs)
{
    bool firstPrint = true;
    cout << "< ";
    foreach(Vertex v, hs) {
        if (firstPrint) {
            firstPrint = false;
        } else {
            cout << ", ";
        }
        cout << boost::get<Handle>(v);
    }
    cout << " >";
}

HandleSeq ForwardChainer::fwdChainSeed(const Handle s, int maxRuleApps)
{
    // 4. For each remaining arg slot of R, find an atom A with 
    // TV.confidence > min_confidence that satisfies the filter, where the probability
    // of A being chosen is pS(A) if A is in the seed stack, and pN(A) if A is not in
    // the seed stack but is in the atom table. Often we choose the functions so that
    // pS(x1) > pN(x2) whenever x1 and x2 are similar in properties. If all slots
    // cannot be filled, set_of_invalid_rules.push(R) and goto 2.
    //
    // 5. If the result of R.compute(with the said args) has
    // confidence < min_confidence, then goto 4. Else, push it into the seed stack.
    // If a target atom template exists and the result satisfies the target atom
    // template or any other optional halt criteria are met, then halt. Else, goto 1. 

    // take the top-most atom from the seed stack and define it as seed atom.
    // then seet the Seed in the RuleProvider (this also clears the
    // set_of_invalid_rules)
    rp.setSeed(s);
    Rule* r = NULL;
    std::vector<Handle> results;
    Handle out;
    while (maxRuleApps && (r = rp.nextRule())) {
        vector<Vertex> cleanArgs;
        int argumentAttempts = 2;
        bool foundArguments;
        int filterSize = r->getInputFilter().size(); 
        cout << "FWDCHAIN Trying rule " << r->name << endl;
        cleanArgs.resize(filterSize);
        cleanArgs[rp.getSeedIndex()] = s;
        HandleSeq choices = getLocalLink(s,cleanArgs);
        do {
            vector<Vertex> args(cleanArgs);
            // Fill slot in the arguments selected by the rule provider
            // for the seed atom...

            argumentAttempts--;
            // Add sufficient links to meet rule arity.
            // TODO: make link selection more efficient or exhaustive
            // TODO: check rule for free input arity and then randomly
            // select arity (exponentially biased to smaller sizes?)
            for (unsigned int i = 0; i < r->getInputFilter().size(); i++) {
                if (i != rp.getSeedIndex()) {
                    // random selection
                    // TODO: sort based on strength and exponential random select
                    Handle randArg;
                    if (choices.size() > 0) {
                        int index = (int) (getRNG()->randfloat() * choices.size() );
                        randArg = choices[index];
                        choices.erase(choices.begin() + index);
                        args[i] = randArg;
                    } else {
                        argumentAttempts = 0;
                    }
                }
            }
            cout << "FWDCHAIN checking if arguments ";
            printVertexVectorHandles(args);
            cout << " are valid... " <<endl;
            if (argumentAttempts > 0)
                foundArguments = r->validate(args);
            if (!foundArguments)
                cout << "FWDCHAIN no" << endl;
            else {
                cout << "FWDCHAIN yes!" << endl;
                cleanArgs = args;
            }

        } while (!foundArguments && argumentAttempts > 1);
        
        if (!foundArguments) {
            cout << "FWDCHAIN No argument combo found for rule " << r->name << endl;
            continue;
        }

        Vertex V=((r->compute(cleanArgs)).GetValue());
        out=boost::get<Handle>(V);
        const TruthValue& tv=GET_ATW->getTV(out);
        //cout<<printTV(out)<<'\n';

        if (!tv.isNullTv() && tv.getCount() > minConfidence) {
            maxRuleApps--;
            results.push_back(out);
            cout<<"Output\n";
            NMPrinter np;
            np.print(out);
        } else {
            // Remove atom if not satisfactory
            GET_ATW->removeAtom(v2h(V));
        }
    }
    return results;
}

HandleSeq ForwardChainer::fwdChainStack(int maxRuleApps)
{
    HandleSeq results;
    // save the length of the stack so that the new seed atoms don't
    // get confused with the ones we're about to process
    int seedStackEnd = seedStack.size();
    //std::deque<Handle> seedStackCopy = seedStack;
    NMPrinter np;

    cout << "FWDCHAIN Forward chaining through stack - " << seedStackEnd << \
        " seeds to try." << endl;
    Handle seed;
    while (maxRuleApps && seedStackEnd > 0 && (seed = seedStack.front())) {
        seedStack.pop_front();
        //opencog::logger().info("Forward chaining on seed %u", seed);
        cout << "FWDCHAIN Forward chaining on seed " << seed << endl;
        np.print(seed);
        HandleSeq hs = fwdChainSeed(seed, 1);
        copy(hs.begin(), hs.end(), back_inserter(results));
        cout << "FWDCHAIN Got " << hs.size() << " results from seed" << endl;
        maxRuleApps--;
        
        // Remove seed from real stack 
        // Note, only remove first instance
        //std::deque<Handle>::iterator i;
        //i = find(seedStack.begin(), seedStack.end(), seed);
        //if (i != seedStack.end()) seedStack.erase(i);
        seedStackEnd--;
        seedStack.push_back(seed);
    }
    return results;

}

} // namespace reasoning
