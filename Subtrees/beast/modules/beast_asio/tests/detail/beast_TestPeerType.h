//------------------------------------------------------------------------------
/*
    This file is part of Beast: https://github.com/vinniefalco/Beast
    Copyright 2013, Vinnie Falco <vinnie.falco@gmail.com>

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef BEAST_TESTPEERTYPE_H_INCLUDED
#define BEAST_TESTPEERTYPE_H_INCLUDED

template <typename Logic, typename Details>
class TestPeerType
    : public Details
    , public Logic
    , public TestPeer
    , public Thread
{
protected:
    // TestPeerDetails
    using Details::get_socket;
    using Details::get_acceptor;
    using Details::get_io_service;

    // Details
    typedef typename Details::protocol_type protocol_type;
    typedef typename Details::socket_type   socket_type;
    typedef typename Details::acceptor_type acceptor_type;
    typedef typename Details::endpoint_type endpoint_type;
    typedef typename Details::resolver_type resolver_type;

    using Details::get_native_socket;
    using Details::get_native_acceptor;
    using Details::get_endpoint;

    // TestPeerLogic
    using Logic::error;
    using Logic::socket;
    using Logic::get_role;
    using Logic::get_model;
    using Logic::on_connect;
    using Logic::on_connect_async;
    using Logic::pure_virtual;

public:
    // Details
    typedef typename Details::arg_type arg_type;
    typedef typename Details::native_socket_type   native_socket_type;
    typedef typename Details::native_acceptor_type native_acceptor_type;

    typedef TestPeerType <Logic, Details> ThisType;

    TestPeerType (arg_type const& arg)
        : Details (arg)
        , Logic (get_socket ())
        , Thread (name ())
    {
    }

    ~TestPeerType ()
    {
    }

    String name () const
    {
        return get_model ().name () + "_" + get_role ().name ();
    }

    void start ()
    {
        startThread ();
    }

    boost::system::error_code join (int timeoutSeconds)
    {
        if (! wait (timeoutSeconds * 1000))
        {
            stopThread (0);
            return error (make_error (errc::timeout));
        }

        return error ();
    }

    //--------------------------------------------------------------------------

    void run ()
    {
        if (get_model () == Model::async)
        {
            if (get_role () == Role::server)
            {
                run_async_server ();
            }
            else if (get_role () == Role::client)
            {
                run_async_client ();
            }
            else
            {
                error () = make_error (errc::unexpected);
            }
        }
        else if (get_model () == Model::sync)
        {
            if (get_role () == Role::server)
            {
                run_sync_server ();
            }
            else if (get_role () == Role::client)
            {
                run_sync_client ();
            }
            else
            {
                error () = make_error (errc::unexpected);
            }
        }
        else
        {
            error () = make_error (errc::unexpected);
        }

        get_io_service ().run ();

        notify ();
    }

    //--------------------------------------------------------------------------

    void run_sync_server ()
    {
        do_listen ();

        if (failure (error ()))
            return;

        if (failure (get_acceptor ().accept (get_socket (), error ())))
            return;

        this->on_connect ();

        if (failure (error ()))
            return ;
    }

    void run_async_server ()
    {
        do_listen ();

        if (failure (error ()))
            return;

        get_acceptor ().async_accept (get_socket (), boost::bind (
            &Logic::on_connect_async, this, boost::asio::placeholders::error));
    }

    //--------------------------------------------------------------------------

    void run_sync_client ()
    {
        if (failure (get_native_socket ().connect (get_endpoint (get_role ()), error ())))
            return;

        this->on_connect ();

        if (failure (error ()))
            return;
    }

    void run_async_client ()
    {
        get_native_socket ().async_connect (get_endpoint (get_role ()),
            boost::bind (&Logic::on_connect_async, this, boost::asio::placeholders::error));
    }

    //--------------------------------------------------------------------------

    void do_listen ()
    {
        if (failure (get_native_acceptor ().open (get_endpoint (get_role ()).protocol (), error ())))
            return;

        // VFALCO TODO Figure out how to not hard code boost::asio::socket_base
        if (failure (get_native_acceptor ().set_option (
                boost::asio::socket_base::reuse_address (true), error ())))
            return;

        if (failure (get_native_acceptor ().bind (get_endpoint (get_role ()), error ())))
            return;

        // VFALCO TODO Figure out how to not hard code boost::asio::socket_base
        if (failure (get_native_acceptor ().listen (
                boost::asio::socket_base::max_connections, error ())))
            return;
    }
};

#endif
