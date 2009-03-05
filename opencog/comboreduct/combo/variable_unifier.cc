#include "ComboReduct/combo/variable_unifier.h"

#include <boost/lexical_cast.hpp>

namespace combo
{

variable_unifier& variable_unifier::DEFAULT_VU()
{
    static combo::variable_unifier * vu = new combo::variable_unifier();
    return *vu;
}

variable_unifier::variable_unifier()
    : updated(false), oneVariableActive(false)
{
}

variable_unifier::variable_unifier(const variable_unifier& unifier)
    : UnifierMap(unifier)
{
    this->oneVariableActive = unifier.isOneVariableActive();
    this->updated = unifier.isUpdated();
}

variable_unifier::~variable_unifier()
{
}

bool variable_unifier::getVariableState(std::string& variable)
{
    UnifierConstIt found_it = find(variable);
    return found_it == end()? false : found_it->second;
}

void variable_unifier::setVariableState(std::string& variable, bool state)
{
    UnifierMap::operator[](variable) = state;
}

void variable_unifier::insert(const std::string& variable, const bool state)
{
    // unifier map already contains the key
    if (find(variable) != end()) {
        return;
    }
    UnifierMap::operator[](variable) = state;
}

bool variable_unifier::contains(const std::string& variable) const
{
    return (find(variable) != end());
}

void variable_unifier::unify(combo::UnifierOperation operation,
                             const variable_unifier& unifier)
{

    // no need to unify empty maps - this should be the default
    if (empty() || unifier.empty() || !unifier.isUpdated()) {
        return;
    }

    if (size() != unifier.size()) {
        // error
        return;
    }

    // reset oneVariableActive state befor unification starts
    this->oneVariableActive = false;

    UnifierIt it = begin();
    UnifierConstIt otherIt = unifier.begin();

    for (; it != end() && otherIt != unifier.end(); it++, otherIt++) {

        if (it->first != otherIt->first) {
            // log
            continue;
        } else {
            // and unification, just AND the states of the variables
            if (operation == combo::UNIFY_AND) {
                it->second = (it->second && otherIt->second);

                if (it->second) {
                    this->oneVariableActive = true;
                }

                this->updated = true;

                // not / or unification, XOR the states of the variables
            } else if (operation == combo::UNIFY_OR  ||
                       operation == combo::UNIFY_NOT ) {
                it->second = (it->second ^ otherIt->second);

                if (it->second) {
                    this->oneVariableActive = true;
                }
            }
        }
    }

    // unification process completed, unifier updated.
    this->updated = true;
}

std::string variable_unifier::toString() const {
    //build mapping str
    std::string res("[ Maping = {");
    for(UnifierConstIt it = begin(); it != end();) {
        res += it->first + std::string("->")
            + boost::lexical_cast<std::string>(it->second);
        ++it;
        if(it != end()) res += ", ";
    }
    res += std::string("}; ");
    //build oneVariableActive str
    res += std::string("oneVariableActive = ")
        + boost::lexical_cast<std::string>(oneVariableActive);
    //build updated str
    res += std::string("; updated = ")
        + boost::lexical_cast<std::string>(updated) + std::string(" ]");
    return res;
}

} // namespace combo
