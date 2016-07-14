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


namespace RaptorQ__v1 {


template <typename Rnd_It, typename Fwd_It>
class RAPTORQ_LOCAL Encoder
{
public:
    // used for precomputation
    Encoder (const uint16_t symbols, const uint16_t symbol_size);
    // with data at the beginning. Less work.
    Encoder (const Rnd_It data_from, const Rnd_It data_to,
                                            const uint16_t symbol_size);

    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> begin ();
    RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> end
														(const uint32_t repair);


	uint64_t add_data (Rnd_It from, const Rnd_It to);
	bool compute_sync();

    std::future<Error> compute();
    uint64_t encode (Fwd_It &output, const Fwd_It end, const uint32_t &id);

private:
    std::unique_ptr<Impl::Encoder<Rnd_It, Fwd_It>> encoder;
};

template <typename In_It, typename Fwd_It>
class RAPTORQ_LOCAL Decoder
{
public:

    using Report = typename Impl::Decoder<In_It, Fwd_It>::Report;

    Decoder (const uint64_t bytes, const uint16_t symbol_size,
															const Report type);

	RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> begin ();
    RaptorQ__v1::It::Decoder::Symbol_Iterator<In_It, Fwd_It> end ();

	Error add_symbol (In_It from, const In_It to, const uint32_t esi);
	using Decoder_Result =typename Impl::Decoder<In_It, Fwd_It>::Decoder_Result;

	bool can_decode() const;
	Decoder_Result decode();
	void stop();

	std::pair<Error, uint16_t> poll() const;
	std::future<std::pair<Error, uint16_t>> wait (bool async) const;

	// return number of symbols.
	// simbol_size % sizeof(FWD) == 0 else assert!
	// returns number of iterators written
	uint64_t decode_symbol (Fwd_It &start, const Fwd_It end,const uint16_t esi);
	std::pair<uint64_t, size_t> decode_bytes (Fwd_It &start, const Fwd_It end,
									const size_t from_byte, const size_t skip);
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
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
												Encoder<Rnd_It, Fwd_It>::begin()
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->begin();
}

template <typename Rnd_It, typename Fwd_It>
RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It>
							Encoder<Rnd_It, Fwd_It>::end (const uint32_t repair)
{
	if (encoder == nullptr)
		return RaptorQ__v1::It::Encoder::Symbol_Iterator<Rnd_It, Fwd_It> (
																	nullptr, 0);
	return encoder->end (repair);
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::add_data (Rnd_It from, const Rnd_It to)
{
	if (encoder == nullptr)
		return 0;
	return encoder->add_data (from, to);
}

template <typename Rnd_It, typename Fwd_It>
bool Encoder<Rnd_It, Fwd_It>::compute_sync()
{
	if (encoder == nullptr)
		return false;
	return encoder->compute_sync();
}

template <typename Rnd_It, typename Fwd_It>
uint64_t Encoder<Rnd_It, Fwd_It>::encode (Fwd_It &output, const Fwd_It end,
															const uint32_t &id)
{
	if (encoder == nullptr)
		return 0;
	return encoder->encode (output, end, id);
}


///////////////////
//// Decoder
///////////////////


template <typename In_It, typename Fwd_It>
Decoder<In_It, Fwd_It>::Decoder (const uint64_t bytes,
								const uint16_t symbol_size, const Report type)
{
	IS_INPUT(In_It, "RaptorQ__v1::Decoder");
	IS_FORWARD(Fwd_It, "RaptorQ__v1::Decoder");

	decoder = std::unique_ptr<Impl::Decoder<In_It, Fwd_It>> (
				new Impl::Decoder<In_It, Fwd_It> (bytes, symbol_size, type));
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
Error Decoder<In_It, Fwd_It>::add_symbol (In_It from, const In_It to,
															const uint32_t esi)
{
	if (decoder == nullptr)
		return Error::INITIALIZATION;
	return decoder->add_symbol (from, to, esi);
}


template <typename In_It, typename Fwd_It>
std::pair<Error, uint16_t> Decoder<In_It, Fwd_It>::poll () const
{
	if (decoder == nullptr)
		return {Error::INITIALIZATION, 0};
	return decoder->poll();
}

template <typename In_It, typename Fwd_It>
std::future<std::pair<Error, uint16_t>> Decoder<In_It, Fwd_It>::wait (
													const bool async) const
{
	if (decoder == nullptr) {
		std::promise<std::pair<Error, uint16_t>> p;
		p.set_value ({Error::INITIALIZATION, 0});
		return p.get_future();
	}
	return decoder->wait (async);
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
uint64_t Decoder<In_It, Fwd_It>::decode_symbol (Fwd_It &start, const Fwd_It end,
															const uint16_t esi)
{
	if (decoder == nullptr)
		return 0;
	return decoder->decode_symbol (start, end, esi);
}

}   // namespace RaptorQ__v1
