/*
 * opencog/cogserver/server/ServerSocket.cc
 *
 * Copyright (C) 2002-2007 Novamente LLC
 * Copyright (C) 2010 Linas Vepstas <linasvepstas@gmail.com>
 * All Rights Reserved
 *
 * Written by Welter Luigi <welter@vettalabs.com>
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

#include <boost/array.hpp>

#include <opencog/cogserver/server/ServerSocket.h>
#include <opencog/util/Logger.h>

using namespace opencog;

ServerSocket::ServerSocket(void) :
    _socket(nullptr), _lineProtocol(true), _closed(false)
{
}

ServerSocket::~ServerSocket()
{
   logger().debug("ServerSocket::~ServerSocket()");
}

bool ServerSocket::isClosed()
{
    return _closed;
}

void ServerSocket::Send(const std::string& cmd)
{
    boost::system::error_code error;
    boost::asio::write(*_socket, boost::asio::buffer(cmd),
                       boost::asio::transfer_all(), error);

    // The most likely cause of an error is that the remote side has
    // closed the socket, and we just don't know it yet.  We should
    // maybe not log those errors?
    if (error && !_closed) {
        logger().warn("ServerSocket::Send(): %s", error.message().c_str());
    }
}

void ServerSocket::SetCloseAndDelete()
{
    if (_closed) return;

    logger().debug("ServerSocket::SetCloseAndDelete()");
    _closed = true;
    _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
    _socket->close();
}

void ServerSocket::SetLineProtocol(bool val)
{
    _lineProtocol = val;
}

bool ServerSocket::LineProtocol()
{
    return _lineProtocol;
}

typedef boost::asio::buffers_iterator<
    boost::asio::streambuf::const_buffers_type> bitter;

// Some random RFC 854 characters
#define IAC 0xff  // Telnet Interpret As Command
#define IP 0xf4   // Telnet IP Interrupt Process
#define AO 0xf5   // Telnet AO Abort Output
#define EL 0xf8   // Telnet EL Erase Line
#define WILL 0xfb // Telnet WILL
#define DO 0xfd   // Telnet DO
#define TIMING_MARK 0x6 // Telnet RFC 860 timing mark
#define TRANSMIT_BINARY 0x0 // Telnet RFC 856 8-bit-clean
#define CHARSET 0x2A // Telnet RFC 2066


// Goal: if the user types in a ctrl-C or a ctrl-D, we want to
// react immediately to this. A ctrl-D is just the ascii char 0x4
// while the ctrl-C is wrapped in a telnet "interpret as command"
// IAC byte secquence.  Basically, we want to forward all IAC
// sequences immediately, as well as the ctrl-D.
//
// Currently not implemented, but could be: support for the arrow
// keys, which generate the sequence 0x1b 0x5c A B C or D.
//
std::pair<bitter, bool>
match_eol_or_escape(bitter begin, bitter end)
{
    bool telnet_mode = false;
    bitter i = begin;
    while (i != end)
    {
        unsigned char c = *i++;
        if (IAC == c) telnet_mode = true;
        if (('\n' == c) ||
            (0x04 == c) || // ASCII EOT End of Transmission (ctrl-D)
            (telnet_mode && (c <= 0xf0)))
        {
            return std::make_pair(i, true);
        }
    }
    return std::make_pair(i, false);
}

void ServerSocket::set_socket(boost::asio::ip::tcp::socket* sock)
{
    _socket = sock;
    _closed = false;
}

void ServerSocket::handle_connection(void)
{
    logger().debug("ServerSocket::handle_connection()");
    OnConnection();
    boost::asio::streambuf b;
    while (not _closed)
    {
        try
        {
            if (LineProtocol())
            {
                boost::asio::read_until(*_socket, b, match_eol_or_escape);
                std::istream is(&b);
                std::string line;
                std::getline(is, line);
                if (!line.empty() && line[line.length()-1] == '\r') {
                    line.erase(line.end()-1);
                }
                OnLine(line);
            }
            else {
                boost::array<char, 128> buf;
                boost::system::error_code error;
                size_t len = _socket->read_some(boost::asio::buffer(buf), error);
                if (error == boost::asio::error::eof)
                    break; // Connection closed cleanly by peer.
                else if (error)
                    throw boost::system::system_error(error); // Some other error.

                OnRawData(buf.data(), len);
            }
        }
        catch (const boost::system::system_error& e)
        {
            if (isClosed()) {
                break;
            } else if (e.code() == boost::asio::error::eof) {
                break;
            } else if (e.code() == boost::asio::error::connection_reset) {
                break;
            } else if (e.code() == boost::asio::error::not_connected) {
                break;
            } else {
                logger().error("ServerSocket::handle_connection(): Error reading data. Message: %s", e.what());
            }
        }
    }

    // Might already be closed from the SetCloseAndelete() method.
    if (not _closed)
    {
       _socket->shutdown(boost::asio::ip::tcp::socket::shutdown_both);
       _socket->close();
    }
    _closed = true;
    delete _socket;
    _socket = nullptr;

    delete this;
}
