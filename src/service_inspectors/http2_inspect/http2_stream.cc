//--------------------------------------------------------------------------
// Copyright (C) 2019-2020 Cisco and/or its affiliates. All rights reserved.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License Version 2 as published
// by the Free Software Foundation.  You may not use, modify or distribute
// this program under any other version of the GNU General Public License.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//--------------------------------------------------------------------------
// http2_stream.cc author Tom Peters <thopeter@cisco.com>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "http2_enum.h"
#include "http2_stream.h"

#include "service_inspectors/http_inspect/http_enum.h"
#include "service_inspectors/http_inspect/http_flow_data.h"
#include "service_inspectors/http_inspect/http_stream_splitter.h"

#include "http2_data_cutter.h"
#include "http2_dummy_packet.h"

using namespace HttpCommon;
using namespace Http2Enums;
using namespace HttpEnums;

Http2Stream::Http2Stream(uint32_t stream_id_, Http2FlowData* session_data_) :
    stream_id(stream_id_),
    session_data(session_data_)
{
}

Http2Stream::~Http2Stream()
{
    delete current_frame;
    if (hi_flow_data)
        session_data->deallocate_hi_memory();
    delete hi_flow_data;
    delete data_cutter[SRC_CLIENT];
    delete data_cutter[SRC_SERVER];
}

void Http2Stream::eval_frame(const uint8_t* header_buffer, int32_t header_len,
    const uint8_t* data_buffer, int32_t data_len, SourceId source_id)
{
    delete current_frame;
    current_frame = Http2Frame::new_frame(header_buffer, header_len, data_buffer,
        data_len, session_data, source_id, this);
    current_frame->update_stream_state();
}

void Http2Stream::clear_frame()
{
    if (current_frame != nullptr) // FIXIT-M why is this needed?
        current_frame->clear();
    delete current_frame;
    current_frame = nullptr;
}

void Http2Stream::set_hi_flow_data(HttpFlowData* flow_data)
{
    assert(hi_flow_data == nullptr);
    hi_flow_data = flow_data;
    session_data->allocate_hi_memory();
}

const Field& Http2Stream::get_buf(unsigned id)
{
    if (current_frame != nullptr)
        return current_frame->get_buf(id);
    return Field::FIELD_NULL;
}

#ifdef REG_TEST
void Http2Stream::print_frame(FILE* output)
{
    if (current_frame != nullptr)
        current_frame->print_frame(output);
}
#endif

Http2DataCutter* Http2Stream::get_data_cutter(HttpCommon::SourceId source_id)
{
    if (!data_cutter[source_id])
        data_cutter[source_id] = new Http2DataCutter(session_data, source_id);
    return data_cutter[source_id];
}

bool Http2Stream::is_open(HttpCommon::SourceId source_id)
{
    return (state[source_id] == STATE_OPEN) || (state[source_id] == STATE_OPEN_DATA);
}

void Http2Stream::finish_msg_body(HttpCommon::SourceId source_id, bool expect_trailers,
    bool clear_partial_buffer)
{
    uint32_t http_flush_offset = 0;
    Http2DummyPacket dummy_pkt;
    dummy_pkt.flow = session_data->flow;
    uint32_t unused = 0;
    const H2BodyState body_state = expect_trailers ?
        H2_BODY_COMPLETE_EXPECT_TRAILERS : H2_BODY_COMPLETE;
    get_hi_flow_data()->finish_h2_body(source_id, body_state, clear_partial_buffer);
    const snort::StreamSplitter::Status scan_result = session_data->hi_ss[source_id]->scan(
        &dummy_pkt, nullptr, 0, unused, &http_flush_offset);
    assert(scan_result == snort::StreamSplitter::FLUSH);
    UNUSED(scan_result);
    session_data->data_processing[source_id] = false;
}
