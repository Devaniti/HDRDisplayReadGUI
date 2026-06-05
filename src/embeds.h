#pragma once

struct MemoryBlock
{
    const void* data;
    size_t size;
};

MemoryBlock GetSignalGeneratorVSBytecode();
MemoryBlock GetSignalGeneratorPSBytecode();
MemoryBlock GetOpenSourceLicensesData();
