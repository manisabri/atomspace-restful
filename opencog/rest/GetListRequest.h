/*
 * opencog/server/ListRequest.h
 *
 * Copyright (C) 2008 by Singularity Institute for Artificial Intelligence
 * All Rights Reserved
 *
 * Written by Gustavo Gama <gama@vettalabs.com>
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

#ifndef _OPENCOG_GET_LIST_REQUEST_H
#define _OPENCOG_GET_LIST_REQUEST_H

#include <sstream>
#include <string>
#include <vector>

#include <opencog/atomspace/types.h>
#include <opencog/server/Request.h>

namespace opencog
{

class GetListRequest : public Request
{

protected:

    std::ostringstream  _output;
    std::string order_by;
    bool descending;
    std::string name;
    Type type;
    bool subtypes;
    HandleSeq requestHandles;

public:

    static inline const RequestClassInfo& info() {
        static const RequestClassInfo _cci(
            "get-list",
            "Get details for a particular atom referred by handle",
            "Usage: get-list handle=<handle> name=<name> type=<type>\n\n"
            "Get details for a particular atom referred by handle\n"
            "   <handle>: list the atom referred by handle (multi-use)\n"
            "   <name>:   name of atom\n"
            "   <type>:   type of atom\n",
            false, true // not shell, is hidden
        );
        return _cci;
    }

    GetListRequest();
    virtual ~GetListRequest();
    virtual bool execute(void);
    virtual bool isShell(void) {return info().is_shell;}
    void sortHandles(HandleSeq& hs, std::string order_by, bool descend = true);
    void makeListHeader();
    void makeOutput(HandleSeq& hs);
    std::string getHTML(std::string);
};


} // namespace 

#endif // _OPENCOG_LIST_REQUEST_H
