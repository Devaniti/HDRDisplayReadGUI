/*
    HDR Display Read GUI
    Copyright (C) 2026  Dmytro Bulatov

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    My contacts are on https://boolka.dev/
*/

#include "precompiled_header.h"

#include "embeds.h"

#include "open_source_licenses.h"
#include "signal_generator_ps.dxbc.h"
#include "signal_generator_vs.dxbc.h"

MemoryBlock GetSignalGeneratorVSBytecode()
{
    return {
        .data = signal_generator_vs_bytecode,
        .size = sizeof(signal_generator_vs_bytecode),
    };
}

MemoryBlock GetSignalGeneratorPSBytecode()
{
    return {
        .data = signal_generator_ps_bytecode,
        .size = sizeof(signal_generator_ps_bytecode),
    };
}

MemoryBlock GetOpenSourceLicensesData()
{
    return {
        .data = open_source_licenses_data,
        .size = sizeof(open_source_licenses_data),
    };
}
