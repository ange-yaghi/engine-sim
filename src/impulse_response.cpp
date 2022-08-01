#include "../include/impulse_response.h"

ImpulseResponse::ImpulseResponse() {
    /* void */
}

ImpulseResponse::~ImpulseResponse() {
    /* void */
}

void ImpulseResponse::initialize(
    const std::string &filename,
    double volume)
{
    m_filename = filename;
    m_volume = volume;
}
