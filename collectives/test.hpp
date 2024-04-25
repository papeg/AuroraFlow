/*
 * Copyright 2024 Gerrit Pape (papeg@mail.upb.de)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

extern "C"
{
    void test(uint32_t collective, uint32_t datatype, uint32_t count, uint32_t iterations, uint32_t dest, int rank, int size, bool check_errors, uint32_t *errors, STREAM<stream_word> &offload_in, STREAM<stream_word> &offload_out);
}