#pragma once

// Standard.
#include <string>
#include <cstdint>
#include <format>

// Custom.
#include "misc/Error.h"

// External.
#include "nameof.hpp"

/** Category of a sound. Mixer channel. */
enum class SoundChannel : uint8_t {
    UI = 0,
    MUSIC,
    ENVIRONMENT,
    VOICE,
    SFX,
    OTHER,

    COUNT, //< Marks the total number of elements in this enum.
};

/**
 * Converts name of a sound channel to enum value.
 *
 * @param sName Name of enum value.
 *
 * @return Enum value.
 */
inline SoundChannel convertSoundChannelNameToEnum(const std::string& sName) {
    if (sName == NAMEOF_ENUM(SoundChannel::UI)) {
        return SoundChannel::UI;
    }
    if (sName == NAMEOF_ENUM(SoundChannel::MUSIC)) {
        return SoundChannel::MUSIC;
    }
    if (sName == NAMEOF_ENUM(SoundChannel::ENVIRONMENT)) {
        return SoundChannel::ENVIRONMENT;
    }
    if (sName == NAMEOF_ENUM(SoundChannel::VOICE)) {
        return SoundChannel::VOICE;
    }
    if (sName == NAMEOF_ENUM(SoundChannel::SFX)) {
        return SoundChannel::SFX;
    }
    if (sName == NAMEOF_ENUM(SoundChannel::OTHER)) {
        return SoundChannel::OTHER;
    }

    Error::showErrorAndThrowException(std::format("unknown sound channel name \"{}\"", sName));
}
