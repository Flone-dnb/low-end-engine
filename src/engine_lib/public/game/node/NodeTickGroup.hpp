#pragma once

/**
 * Describes the order of ticking. Every object of the first tick group will tick first
 * then objects of the second tick group will tick and etc. This allows defining a special
 * order of ticking if some tick functions depend on another or should be executed first/last.
 *
 * Here, "ticking" means calling a function that should be called every frame.
 */
enum class TickGroup : unsigned char { FIRST, SECOND };
