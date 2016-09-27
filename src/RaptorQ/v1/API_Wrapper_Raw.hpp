/*
 * Copyright (c) 2015-2016, Luca Fulchir<luca@fulchir.it>, All rights reserved.
 *
 * This file is part of "libRaptorQ".
 *
 * libRaptorQ is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * libRaptorQ is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * and a copy of the GNU Lesser General Public License
 * along with libRaptorQ.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "RaptorQ/v1/RaptorQ.hpp"
#include "RaptorQ/v1/RaptorQ_Iterators.hpp"
#include <future>
#include <utility>


namespace RaptorQ__v1 {

template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_API Encoder
{
public:
    // used for precomputation
    Encoder (const uint16_t symbols, const uint16_t symbol_size);
    // with data at the beginning. Less work.
    Encoder (const Rnd_It data_from, const Rnd_It data_to,
													const uint16_t symbol_size);

	uint16_t symbols() const;
	uint16_t symbol_size() const;
	uint32_t max_repair() const;

	RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_source();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin_repair();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end_repair
														(const uint32_t repair);

	uint64_t add_data (Rnd_It &from, const Rnd_It to);
	void clear_data();
	bool compute_sync();
	uint64_t needed_bytes();

    std::future<Error> compute();
    uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);

private:
    std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> encoder;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_API Decoder
{
public:

    using Report = typename Impl::Decoder<In_It, Fwd_It>::Report;

    Decoder (const uint16_t symbols, const uint16_t symbol_size,
															const Report type);

	uint16_t symbols() const;
	uint16_t symbol_size() const;

	RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> begin ();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> end ();

	Error add_symbol (In_It &from, const In_It to, const uint32_t esi);
	using Decoder_Result =typename Impl::Decoder<In_It, Fwd_It>::Decoder_Result;

	bool can_decode() const;
	Decoder_Result decode();
	void stop();
	uint16_t needed_symbols() const;

	std::pair<Error, uint16_t> poll();
	std::pair<Error, uint16_t> wait_sync();
	std::future<std::pair<Error, uint16_t>> wait();

	Error decode_symbol (Fwd_It &start, const Fwd_It end,const uint16_t esi);
	// returns numer of bytes written, offset of data in last iterator
	std::pair<uint64_t, size_t> decode_bytes (Fwd_It &start, const Fwd_It end,
									const uint64_t from_byte, const size_t skip);
private:
    std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> decoder;
};




///////////////////
//// Encoder
///////////////////


template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const uint16_t symbols,
													const uint16_t symbol_size)
{
	IS_RANDOM(Rnd_It, "RaptorQ__v1::Encoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Encoder");
	encoder = std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> (
					new Impl::Encoder<Rnd_It, Fwd_It> (symbols, symbol_size));
}

template <typename Rnd_It, typename Fwd_It>
Encoder<Rnd_It, Fwd_It>::Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t symbol_size)
{
	IS_RANDOM(Rnd_It, "RaptorQ__v1::Encoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Encoder");
	encoder = std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> (
						new Impl::Encoder<Rnd_It, Fwd_It> (data_from, data_to,
																symbol_size));
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbols() const
{
	if (encoder == nullptr)
		return 0;
	return encoder->symbols();
}

template <typename Rnd_It, typename Fwd_It>
uint16_t Encoder<Rnd_It, Fwd_It>::symbol_size() const
{
	if (encoder == nullptr)
		return 0;
	return encoder->symbol_size();
}

template <typename Rnd_It, typename Fwd_It>
uint32_t Encoder<Rnd_It, Fwd_It>::max_repair() const
{
	if (encoder == nullptr)
		return 0;
	return encoder->max_repair();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::begin_source()
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->begin_source();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::end_source ()
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->end_source();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
										Encoder<Rnd_It, Fwd_It>::begin_repair()
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->begin_repair();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
					Encoder<Rnd_It, Fwd_It>::end_repair (const uint32_t repair)
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->end_repair (repair);
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::add_data (Rnd_It &from, const Rnd_It to)
{
	if (encoder == nullptr)
		return 0;
	return encoder->add_data (from, to);
}

template <typename Rnd_It, typename Fwd_It>
void Encoder<Rnd_It, Fwd_It>::clear_data()
{
	if (encoder == nullptr)
		return;
	encoder->clear_data();
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
{
	if (encoder == nullptr)
		return false;
	return encoder->compute_sync();
}

template <typename Rnd_It, typename Fwd_It>
std::future<Error> Encoder<Rnd_It, Fwd_It>::compute()
{
	if (encoder == nullptr) {
		std::promise<Error> p;
		p.set_value (Error::INITIALIZATION);
		return p.get_future();
	}
	return encoder->compute();
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
															const uint32_t &id)
{
	if (encoder == nullptr)
		return 0;
	return encoder->encode (output, end, id);
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::needed_bytes ()
{
	if (encoder == nullptr)
		return 0;
	return encoder->needed_bytes ();
}

///////////////////
//// Decoder
///////////////////


template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const uint16_t symbols,
								const uint16_t symbol_size, const Report type)
{
	IS_INPUT(In_It, "RaptorQ__v1::Decoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");

	decoder = std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> (
				new Impl::Decoder<In_It, Fwd_It> (symbols, symbol_size, type));
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbols() const
{
	if (decoder == nullptr)
		return 0;
	return decoder->symbols();
}

template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::symbol_size() const
{
	if (decoder == nullptr)
		return 0;
	return decoder->symbol_size();
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
												Decoder<In_It, Fwd_It>::begin()
{
	if (decoder == nullptr)
		return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
																	nullptr, 0);
	return decoder->begin();
}

template <typename In_It, typename Fwd_It>
RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It>
												Decoder<In_It, Fwd_It>::end()
{
	if (decoder == nullptr)
		return RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> (
																	nullptr, 0);
	return decoder->end();
}


template <typename In_It, typename Fwd_It>
uint16_t Decoder<In_It, Fwd_It>::needed_symbols() const
{
	if (decoder == nullptr)
		return 0;
	return decoder->needed_symbols();
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::add_symbol (In_It &from, const In_It to,
															const uint32_t esi)
{
	if (decoder == nullptr)
		return Error::INITIALIZATION;
	return decoder->add_symbol (from, to, esi);
}


template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::poll ()
{
	if (decoder == nullptr)
		return {Error::INITIALIZATION, 0};
	return decoder->poll();
}

template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::wait_sync()
{
	if (decoder == nullptr) {
		return {Error::INITIALIZATION, 0};
	}
	return decoder->wait_sync();
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint16_t>> Decoder<In_It, Fwd_It>::wait()
{
	if (decoder == nullptr) {
		std::promise<std::pair<Error, uint16_t>> p;
		p.set_value ({Error::INITIALIZATION, 0});
		return p.get_future();
	}
	return decoder->wait();
}

template <typename In_It, typename Fwd_It>
bool Decoder<In_It, Fwd_It>::can_decode() const
{
	if (decoder == nullptr)
		return false;
	return decoder->can_decode();
}

template <typename In_It, typename Fwd_It>
typename Decoder<In_It, Fwd_It>::Decoder_Result Decoder<In_It, Fwd_It>::decode()
{
	if (decoder == nullptr)
		return Decoder_Result::NEED_DATA;
	return decoder->decode();
}

template <typename In_It, typename Fwd_It>
void Decoder<In_It, Fwd_It>::stop()
{
	if (decoder != nullptr)
		decoder->stop();
}

template <typename In_It, typename Fwd_It>
std::pair<uint64_t, size_t> Decoder<In_It, Fwd_It>::decode_bytes (Fwd_It &start,
													const Fwd_It end,
													const uint64_t from_byte,
													const size_t skip)
{
	if (decoder == nullptr)
		return {0, skip};
	return decoder->decode_bytes (start, end, from_byte, skip);
}

template <typename In_It, typename Fwd_It>
Error Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
															const uint16_t esi)
{
	if (decoder == nullptr)
		return Error::INITIALIZATION;
	return decoder->decode_symbol (start, end, esi);
}

}   // namespace RaptorQ__v1
