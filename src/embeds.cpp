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
