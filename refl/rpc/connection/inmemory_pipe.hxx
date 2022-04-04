/*******************************************************************************
 * MIT License
 *
 * Copyright (c) 2022. Seungwoo Kang
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * project home: https://github.com/perfkitpp
 ******************************************************************************/

#pragma once
#include "../../../circular_queue.hxx"
#include "../../../spinlock.hxx"
#include "../../__namespace__"
#include "../connection.hxx"

namespace CPPHEADERS_NS_::rpc::connection {
class inmemory_pipe : public if_connection_streambuf
{
    struct pipe {
        spinlock             lock;
        circular_queue<char> strm{1024};
        size_t               total{0};

        inmemory_pipe*       receiver;
    };

   private:
    int              _idx = 0;
    shared_ptr<pipe> _in, _out;

    char             _ibuf[2048];
    char             _obuf[2048];

    std::atomic_flag _no_signal;

   private:
    inmemory_pipe()
            : if_connection_streambuf("INMEMORY" + std::to_string((intptr_t)this))
    {
        _no_signal.test_and_set();
    }

   public:
    static auto create()
    {
        unique_ptr<inmemory_pipe> insts[2];
        shared_ptr<pipe>          pipes[2];

        for (auto& p : insts) { p.reset(new inmemory_pipe); }
        for (auto& p : pipes) { p = std::make_shared<pipe>(); }

        {
            auto& [ia, ib] = insts;
            auto& [pa, pb] = pipes;
            ia->_in = ib->_out = pa;
            ia->_out = ib->_in = pb;

            pa->receiver = &*ia;
            pb->receiver = &*ib;
        }

        return std::make_pair(std::move(insts[0]), std::move(insts[1]));
    }

   public:
    void initialize() noexcept override
    {
    }

    void start_data_receive() noexcept override
    {
        lock_guard _lc_{_in->lock};

        if (not _in->strm.empty())
            this->on_data_receive();
        else
            _no_signal.clear();
    }

    void close() noexcept override
    {
        // TODO: Set pipe's 'receiver' field nullptr.
        // TODO: Change 'receiver' access atomic, and be checked.
    }

    void get_total_rw(size_t* num_read, size_t* num_write) override
    {
        {
            lock_guard _{_in->lock};
            *num_read = _in->total;
        }
        {
            lock_guard _{_out->lock};
            *num_write = _out->total;
        }
    }

   protected:
    int_type overflow(int_type int_type) override
    {
        _do_sync();

        *_obuf = traits_type::to_char_type(int_type);
        pbump(1);

        return int_type;
    }

    int_type underflow() override
    {
        // TODO: Wait until valid data receive
        return basic_streambuf::underflow();
    }

    int sync() override
    {
        _do_sync();

        return basic_streambuf::sync();
    }

   private:
    void _do_sync()
    {
        auto wbeg = pbase();
        auto wend = pptr();
        auto nwrite = wend - wbeg;

        if (nwrite > 0) {
            lock_guard _lc_{_out->lock};
            auto       strm = &_out->strm;

            if (strm->capacity() - strm->size() < nwrite)
                strm->reserve_shrink(std::max(strm->capacity() * 2, strm->capacity() + nwrite));

            strm->enqueue_n(wbeg, nwrite);
            _out->total += nwrite;

            auto recv = _out->receiver;
            if (not recv->_no_signal.test_and_set())
                recv->on_data_receive();
        }

        setp(_obuf, _obuf + sizeof _obuf);
    }
};

}  // namespace CPPHEADERS_NS_::rpc::connection