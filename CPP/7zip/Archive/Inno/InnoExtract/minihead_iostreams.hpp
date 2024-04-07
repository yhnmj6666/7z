#pragma once
#ifndef _MINIHEAD_IOSTREAMS_HPP
#define _MINIHEAD_IOSTREAMS_HPP

#include "minihead.hpp"

#include <type_traits>

namespace boost {
namespace iostreams {

enum flag_type {
	f_read = 1,
	f_write = f_read << 1,
	f_eof = f_write << 1,
	f_good,
	f_would_block
};

constexpr int WOULD_BLOCK = -2;
constexpr ::std::streamsize default_device_buffer_size = 4096;

struct basic_mini_buffer : std::streambuf
{
	using streamsize = std::streamsize;
	using ios_base = std::ios_base;
	using buffer_type = char*;
	std::shared_ptr<char[]> _buffer_area;
	streamsize _size;
	char* _ptr, * _eptr;
	bool _eof;
	basic_mini_buffer(streamsize buffer_size)
	{
		_size = buffer_size;
		_buffer_area = std::shared_ptr<char[]>(new char[(uint32_t)_size]);
		_ptr = _buffer_area.get();
		_eptr = _buffer_area .get()+ _size;
		_eof = false;
		setp(nullptr, nullptr);
		setg(_buffer_area.get(), _buffer_area.get(), _buffer_area.get());
	}

	virtual int underflow()
	{
		using namespace std;
		streamsize _pback_size = 0;
		if (!gptr())
			setg(_buffer_area.get(), _buffer_area.get(), _buffer_area.get());
		if (gptr() < egptr()) return traits_type::to_int_type(*gptr());

		// Fill putback buffer.
		std::streamsize keep =
			(std::min)(static_cast<std::streamsize>(gptr() - eback()),
				_pback_size);
		if (keep)
			traits_type::move(data() + (_pback_size - keep),
				gptr() - keep, (size_t)keep);

		// Set pointers to reasonable values in case read throws.
		setg(data() + _pback_size - keep,
			data() + _pback_size,
			data() + _pback_size);

		// Read from source.
		std::streamsize chars =
			readmore(data() + _pback_size, size() - _pback_size);
		if (chars == -1) {
			_eof = true;
			chars = 0;
		}
		setg(eback(), gptr(), data() + _pback_size + chars);
		return chars != 0 ?
			traits_type::to_int_type(*gptr()) :
			traits_type::eof();
	}

	virtual streamsize readmore(char* /*s*/, streamsize /*n*/)
	{
		return streamsize(-1);
	}

	void set(streamsize ptr, streamsize end)
	{
		_ptr = data() + ptr;
		_eptr = data() + end;
	}

	bool eof() const { return _eof; }
	char* begin() const { return _buffer_area.get(); }
	char* in() const { return _buffer_area.get(); }
	char* end() const { return _buffer_area .get() + _size; }
	char* data() const { return _buffer_area.get(); }
	std::streamsize size() const { return _size; }
	char*& ptr() { return _ptr; }
	char*& eptr() { return _eptr; }
};

template<class T>
struct mini_buffer : basic_mini_buffer
{
	T& _t;
	mini_buffer(streamsize buffer_size, T& t) : _t(t), basic_mini_buffer(buffer_size) {}
	virtual streamsize readmore(char* s, streamsize n) override
	{
		return _t.readmore(s, n);
	}
};

struct input {
	basic_mini_buffer _buf;

	input(std::streamsize buffer_size = default_device_buffer_size) : _buf(buffer_size) {}


	//void register_buffer(basic_mini_buffer* b) { _buf = b; }
	basic_mini_buffer& buf() { return _buf; }
};

struct basic_linked_filter {
	basic_linked_filter* _next = nullptr;
	using ios_base=std::ios_base;
	using pos_type = std::streambuf::pos_type;
	using off_type=std::streambuf::off_type;

	virtual ::std::streamsize read(char* /*dest*/, ::std::streamsize /*n*/) = 0;
	virtual int get() = 0;
	virtual ~basic_linked_filter() {};
};

struct source : public boost::iostreams::input
{
	explicit source(std::streamsize buffer_size = boost::iostreams::default_device_buffer_size) {
		//minihead_log_func(__FUNCSIG__);
		(void)buffer_size;
	}

	::std::streamsize read(char* dest, ::std::streamsize n)
	{
		minihead_log_func(__FUNCSIG__);
		(void)dest;
		(void)n;
		return ::std::streamsize(0);
	}
};

template<typename Source>
::std::streamsize read(Source* src, char* buffer, ::std::streamsize desired)
{
	return src->read(buffer, desired);
}

template<typename Source>
::std::streamsize read(Source& src, char* buffer, ::std::streamsize desired)
{
	return src.read(buffer, desired);
}

template<typename Source>
int get(Source* src)
{
	return src->get();
}

template<class T>
struct filter_wrapper : basic_linked_filter
{
	filter_wrapper(const T& t, std::streamsize buffer_size) : _t(t), _buf(buffer_size, *this) {
		//_t.register_buffer(&_buf);
	}
	virtual ::std::streamsize read(char* dest, ::std::streamsize n) override
	{
		std::streamsize r=_buf.sgetn(dest, n);
		if (r == 0 && _buf.eof())
			return std::streamsize(-1);
		return r;
	}
	virtual int get()
	{
		return _buf.sbumpc();
	}
	::std::streamsize readmore(char* dest, ::std::streamsize n)
	{
		return read(dest, n, _next);
	}
	template<typename Source>
	std::streamsize read(char* s, std::streamsize n, Source* src)
	{
		return _t.read(src, s, n);
	}
	T _t;
	mini_buffer<filter_wrapper> _buf;
};

template<class T>
struct source_wrapper : basic_linked_filter
{
	source_wrapper(const T& t, std::streamsize buffer_size) : _t(t), _buf(buffer_size, *this) {
		//_t.register_buffer(&_buf);
	}
	virtual ::std::streamsize read(char* dest, ::std::streamsize n) override
	{
		std::streamsize r = _buf.sgetn(dest, n);
		if (r == 0 && _buf.eof())
			return std::streamsize(-1);
		return r;
	}
	virtual int get()
	{
		return _buf.sbumpc();
	}
	::std::streamsize readmore(char* dest, ::std::streamsize n)
	{
		return _t.read(dest, n);
	}
	T _t;
	mini_buffer<source_wrapper> _buf;
};

template <class Impl, class Allocator = std::allocator<char> >
struct symmetric_filter : public boost::iostreams::input
{
	int _state;
	Impl _impl;
	symmetric_filter(::std::streamsize buffer_size)
		: _state(0), input(buffer_size), _impl(Impl())
	{}

	template<typename Source>
	std::streamsize read(Source& src, char* s, std::streamsize n)
	{
		using namespace std;
		if (!(state() & f_read))
		{
			state() |= f_read;
			buf().set(0, 0);
		}

		basic_mini_buffer& buf = this->buf();
		int status = (state() & f_eof) != 0 ? f_eof : f_good;
		char* next_s = s,
			* end_s = s + n;
		while (true)
		{
			// Invoke filter if there are unconsumed characters in buffer or if
			// filter must be flushed.
			bool flush = status == f_eof;
			if (buf.ptr() != buf.eptr() || flush) {
				const char* next = buf.ptr();
				bool done =
					!filter().filter(next, buf.eptr(), next_s, end_s, flush);
				buf.ptr() = buf.data() + (next - buf.data());
				if (done)
					return static_cast<std::streamsize>((next_s - s) != 0 ? n : -1);
			}

			// If no more characters are available without blocking, or
			// if read request has been satisfied, return.
			if ((status == f_would_block && buf.ptr() == buf.eptr()) ||
				next_s == end_s)
			{
				return static_cast<std::streamsize>(next_s - s);
			}

			// Fill buffer.
			if (status == f_good)
				status = fill(src);
		}
	}

	template<typename Source>
	int fill(Source& src)
	{
		std::streamsize amt = iostreams::read(src, buf().data(), buf().size());
		if (amt == -1) {
			state() |= f_eof;
			return f_eof;
		}
		buf().set(0, amt);
		return amt != 0 ? f_good : f_would_block;
	}

	int& state() { return _state; }
	Impl& filter() { return _impl; }
};

// TODO: finish this
struct zlib_decompressor : public boost::iostreams::input
{
	explicit zlib_decompressor(int buffer_size = boost::iostreams::default_device_buffer_size) {
		minihead_log_func(__FUNCSIG__);
		(void)buffer_size;
	}

	template <typename Source>
	::std::streamsize read(Source& src, char* dest, ::std::streamsize n)
	{
		minihead_log_func(__FUNCSIG__);
		(void)src;
		(void)dest;
		(void)n;
		return ::std::streamsize(0);
	}
};

// TODO: finish this
struct bzip2_decompressor : public boost::iostreams::input
{
	explicit bzip2_decompressor(int buffer_size = boost::iostreams::default_device_buffer_size) {
		minihead_log_func(__FUNCSIG__);
		(void)buffer_size;
	}

	template <typename Source>
	::std::streamsize read(Source& src, char* dest, ::std::streamsize n)
	{
		minihead_log_func(__FUNCSIG__);
		(void)src;
		(void)dest;
		(void)n;
		return ::std::streamsize(0);
	}
};

struct restrict : public boost::iostreams::source
{
	::std::istream& _is;
	::std::streamsize _consumed;
	::std::streamsize _len;

	explicit restrict(::std::istream& is, ::std::streamoff off, ::std::streamoff len = boost::iostreams::default_device_buffer_size)
		: _is(is), _consumed(0), _len(len)
	{
		_is.seekg(off, ::std::ios_base::cur);
	}

	::std::streamsize read(char* dest, ::std::streamsize n)
	{
		if (n == 0)
			return ::std::streamsize(0);
		::std::streamsize remaining = std::min(_len - _consumed, n);
		if (remaining <= 0)
			return ::std::streamsize(-1);
		::std::streampos pre_read = _is.tellg();
		_is.read(dest, remaining);
		::std::streampos post_read = _is.tellg();
		_consumed += (post_read - pre_read);
		return (post_read - pre_read) != 0 ? post_read - pre_read : ::std::char_traits<char>::eof();
	}
};

struct multichar_input_filter : public boost::iostreams::input
{
	using char_type = char;
	using category = void;

	template <typename Source>
	::std::streamsize read(Source& src, char* dest, ::std::streamsize n)
	{
		minihead_log_func(__FUNCSIG__);
		(void)src;
		(void)dest;
		(void)n;
		return ::std::streamsize(0);
	}
};

template<class type>
struct chain : public ::std::streambuf {
	::std::vector<basic_linked_filter*> _filters;

	virtual ~chain()
	{
		for (auto& f : _filters)
			delete f;
	}

	template <typename T>
	void push(const T& t, ::std::streamsize buffer_size = default_device_buffer_size)
	{
		basic_linked_filter* wrapped = nullptr;
		if constexpr (::std::is_convertible_v<T, source>)
		{
			using wrapped_source = source_wrapper<T>;
			wrapped = new wrapped_source(t, buffer_size);
		}
		else
		{
			using wrapped_filter = filter_wrapper<T>;
			wrapped = new wrapped_filter(t, buffer_size);
		}
		if (!_filters.empty())
		{
			basic_linked_filter* prevF = _filters.back();
			prevF->_next = wrapped;
		}
		_filters.emplace_back(wrapped);
	}
	template <typename Source>
	::std::streamsize read(Source& /*src*/, char* dest, ::std::streamsize n)
	{
		if (_filters.empty())
		{
			return std::streamsize(0);
		}
		else
		{
			_total_read += n;
			return _filters.front()->read(dest, n);
		}
	}
	::std::streamsize read(char* dest, ::std::streamsize n)
	{
		if (_filters.empty())
		{
			return std::streamsize(0);
		}
		else
		{
			_total_read += n;
			return _filters.front()->read(dest, n);
		}
	}
	
	std::streamsize TotalRead() { return _total_read; }
protected:
	std::streamsize _total_read = 0;
	virtual std::streamsize xsgetn(char_type* s, std::streamsize count) override
	{
		_total_read += size_t(count);
		return read(s, count);
	}
};

struct filtering_istream : public ::std::istream, public chain<char>
{
	filtering_istream() : ::std::istream((chain*)(this)) {}
	void exceptions(::std::ios_base::iostate state);
	
	template <typename Source>
	::std::streamsize read(Source& src, char* dest, ::std::streamsize n)
	{
		minihead_log_func(__FUNCSIG__);
		(void)src;
		(void)dest;
		(void)n;
		return ::std::streamsize(0);
	}
};

}
}

#endif