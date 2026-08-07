#include "../include/bov_sound.h"

BovSound::BovSound() {
    m_volume = 1.0;
}

BovSound::~BovSound() {
    /* void */
}

void BovSound::initialize(
    const std::string &filename,
    double volume)
{
    m_filename = filename;
    m_volume = volume;
}
