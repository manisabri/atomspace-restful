/*
 * opencog/learning/moses/eda/field_set.cc
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * All Rights Reserved
 *
 * Written by Moshe Looks
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include "field_set.h"
#include <sstream>
#include <opencog/util/dorepeat.h>
#include <opencog/util/exceptions.h>
#include <opencog/util/oc_assert.h>
#include <opencog/util/iostreamContainer.h>

namespace opencog {
namespace moses {

const disc_t field_set::contin_spec::Stop = 0;
const disc_t field_set::contin_spec::Left = 1;
const disc_t field_set::contin_spec::Right = 2;

const disc_t field_set::term_spec::Stop = 0;

field_set& field_set::operator=(const field_set& rhs)
{
    _fields = rhs._fields;
    _disc = rhs._disc;
    _contin = rhs._contin;
    _term = rhs._term;
    _nbool = rhs._nbool;
    _contin_start = _fields.begin() + distance(rhs._fields.begin(),
                                               rhs._contin_start);
    _disc_start = _fields.begin() + distance(rhs._fields.begin(),
                                             rhs._disc_start);
    return *this;
}
bool field_set::operator==(const field_set& rhs) const
{
    return (_disc == rhs._disc && _contin == rhs._contin &&
            _term == rhs._term && _nbool == rhs._nbool);
}

const term_t& field_set::get_term(const instance& inst, size_t idx) const
{
    size_t raw_idx = term_to_raw_idx(idx);

    // Walk down the tree to get the appropriate term_t.
    const term_spec& o = _term[idx];
    term_tree::iterator it = o.tr->begin();
    for (size_t i = 0; i < o.depth; ++i) {
        disc_t raw_value = get_raw(inst, raw_idx + i);
        if (raw_value == term_spec::Stop) {
            break;
        } else {
            it = o.tr->child(it, term_spec::to_child_idx(raw_value));
        }
    }
    return *it;
}

contin_t field_set::get_contin(const instance& inst, size_t idx) const
{
    size_t raw_idx = contin_to_raw_idx(idx);

    // Start with the mean, and walk down to the res.
    const contin_spec& c = _contin[idx];
    contin_stepper stepper(c);
    for (size_t i = 0; i < c.depth; ++i) {
        disc_t direction = get_raw(inst, raw_idx + i);
        if (direction == contin_spec::Stop) {
            break;
        } else if (direction == contin_spec::Left) {
            stepper.left();
        } else { //direction==contin_spec::Right
            OC_ASSERT(direction == contin_spec::Right,
                             "direction should be set to contin_spec::Right (get_contin).");
            stepper.right();
        }
    }
    return stepper.value;
}

void field_set::set_contin(instance& inst, size_t idx, contin_t target) const
{
    size_t raw_idx = contin_to_raw_idx(idx);
    const contin_spec& c = _contin[idx];

    // Use binary search to assign to the nearest value.
    contin_t best_distance = fabs(c.mean - target);
    size_t best_depth = 0;
    contin_stepper stepper(c);
    for (width_t i = 0; i<c.depth && best_distance>c.epsilon(); ++i) {
        // Take a step in the correct direction.
        if (target < stepper.value) { //go left
            stepper.left();
            set_raw(inst, raw_idx + i, contin_spec::Left);
        } else { //target>stepper.value - go right
            stepper.right();
            set_raw(inst, raw_idx + i, contin_spec::Right);
        }
        if (fabs(stepper.value - target) < best_distance) {
            best_distance = fabs(stepper.value - target);
            best_depth = i + 1;
        }
    }

    // Backtrack up to the best depth.
    for (size_t i = best_depth; i < c.depth; ++i)
        set_raw(inst, raw_idx + i, contin_spec::Stop);
}

//note to self (Moshe): changed this on 4/23 - its confusing that
//stream and stream_raw give different orderings among the various
//types of vars - what was I thinking?
std::string field_set::stream(const instance& inst) const
{
    std::stringstream ss;
    ss << "[";
    ostreamContainer(ss, begin_term(inst), end_term(inst), "#", "#", "", false);
    ostreamContainer(ss, begin_contin(inst), end_contin(inst), "|", "|", "", false);
    ostreamContainer(ss, begin_disc(inst), end_disc(inst), " ", " ", "", false);
    ostreamContainer(ss, begin_bits(inst), end_bits(inst), "", "", "", false);
    ss << "]";
    return ss.str();
}

std::string field_set::stream_raw(const instance& inst) const
{
    std::stringstream ss;
    ostreamContainer(ss, begin_raw(inst), end_raw(inst), "", "[", "]");
    return ss.str();
}

void field_set::build_spec(const spec& s, size_t n)
{
    if (const term_spec* os = boost::get<term_spec>(&s)) {
        build_term_spec(*os, n);
    } else if (const contin_spec* cs = boost::get<contin_spec>(&s)) {
        build_contin_spec(*cs, n);
    } else if (const disc_spec* ds = boost::get<disc_spec>(&s)) {
        build_disc_spec(*ds, n);
    } else {
        OC_ASSERT(false, "This spec is NULL or unknown");
    }
}

void field_set::build_disc_spec(const disc_spec& ds, size_t n)
{
    width_t width = nbits_to_pack(ds.multy);
    size_t base = back_offset();
    for (size_t idx = 0;idx < n;++idx)
        _fields.push_back(field(width,
                                (base + idx*width) / bits_per_packed_t,
                                (base + idx*width) % bits_per_packed_t));
    _disc.insert(_disc.end(), n, ds);
    if (width == 1)
        _nbool += n;
}

void field_set::build_contin_spec(const contin_spec& cs, size_t n)
{
    // Depth must be a power of 2
    // XXX Really??? Why?? Elswhere we seem to treat "depth" as
    // the depth of the tree (see e.g. use of "best_depth" which
    // increments by one. Yet here, its used as the breadth of
    // the tree, not the dpeth. See, for example, epsilon()
    // where depth is used as a shift...
    OC_ASSERT(cs.depth == next_power_of_two(cs.depth),
              "depth must be a power of 2 and it is %d",
              cs.depth);
    //all have arity of 3 (left, right, or stop) and hence are 2 wide
    size_t base = back_offset(), width = 2;
    dorepeat (n*cs.depth) {
        _fields.push_back(field(width, base / bits_per_packed_t,
                                base % bits_per_packed_t));
        base += width;
    }
    _contin.insert(_contin.end(), n, cs);
}

void field_set::build_term_spec(const term_spec& os, size_t n)
{
    size_t base = back_offset(), width = nbits_to_pack(os.branching);
    size_t total_width = size_t((width * os.depth - 1) /
                                bits_per_packed_t + 1) * bits_per_packed_t;

    dorepeat (n) {
        dorepeat (os.depth) {
            _fields.push_back(field(width, base / bits_per_packed_t,
                                    base % bits_per_packed_t));
            base += width;
        }
        base += total_width - (os.depth * width); //term vars must pack evenly
    }
    _term.insert(_term.end(), n, os);
}

std::ostream& field_set::ostream_field(std::ostream& out, field_iterator fit) const
{
    if (fit < end_term_fields())
    {
       unsigned idx = fit - begin_term_fields();
       const term_spec &s = _term[idx];
       out << "{ type=term"
           << "; idx=" << idx
           << "; depth=" << s.depth
           << "; branching=" << s.branching
           << "; }";
    }
    else if (fit < end_contin_fields())
    {
       unsigned idx = fit - begin_contin_fields();
       const contin_spec &s = _contin[idx];
       out << "{ type=contin"
           << "; idx=" << idx
           << "; depth=" << s.depth
           << "; mean=" << s.mean
           << "; step_size=" << s.step_size
           << "; expansion=" << s.expansion
           << "; }";
    }
    else if (fit < end_disc_fields())
    {
       unsigned idx = fit - begin_disc_fields();
       const disc_spec &s = _disc[idx];
       out << "{ type=disc"
           << "; idx=" << idx
           << "; multiplicity=" << s.multy
           << "; }";
    }
    else
    {
       out << "{ type=bit; }";
    }

    return out;
}

std::ostream& field_set::ostream_field_set(std::ostream& out) const
{
    using std::endl;

    // Use a pseudo-json style printout.
    out << "field_set = {" << endl;

    out << "n_term= " << n_term()
        << "; n_contin= " << n_contin()
        << "; n_disc= " << n_disc()
        << "; n_bits= " << n_bits()
        << ";" << endl;

    out << "fields = {" << endl;

    field_iterator fit = _fields.begin();
    unsigned idx = 0;
    for (; fit != _fields.end(); fit++, idx++)
    {
        out << "{ idx=" << idx << "; ";

        // print the raw field locations
        const field &f = *fit;
        out << "width=" << f.width
            << "; major=" << f.major_offset
            << "; minor=" << f.minor_offset
            << "; field=";

        ostream_field(out, fit);
        out << "}," << endl;
    }
    out << "}; };" << endl;
    return out;
}


} // ~namespace moses
} // ~namespace opencog
