/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  Copyright © 2017 Videolabs                                              #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicProcessor.cpp                                   #*/
/*#  Version:       0.2                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Peter Stitt                                              #*/
/*#  Licence:       LGPL                                                     #*/
/*#                                                                          #*/
/*############################################################################*/

// Decoder coefficients optimised for 5.0 playback system.
// Coefficients from Bruce Wiggins, University of Derby.
// Works up to maximum of second order, even with third order inputs
const float decoder_coefficient_5_0[][16] =
{
{0.2864f, 0.3100f, 0.f, 0.3200f, 0.1443f, 0.f, 0.f, 0.f, 0.0981f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{0.2864f, -0.3100f, 0.f, 0.3200f, -0.1443f, 0.f, 0.f, 0.f, 0.0981f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{0.4490f, 0.2800f, 0.f, -0.3350f, 0.0924f, 0.f, 0.f, 0.f, -0.0924f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{0.4490f, -0.2800f, 0.f, -0.3350f, -0.0924f, 0.f, 0.f, 0.f, -0.0924f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{0.0601f, 0.f, 0.f, 0.0400f, 0.f, 0.f, 0.f, 0.f, 0.0520f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{1.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}
};

// Decoder coefficients for Ambisonics to stereo. Useful for conversion to 2-channels when not using headphone.
const float decoder_coefficient_stereo[][16] =
{
{0.5f, 0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f},
{0.5f, -0.5f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f, 0.f}
};
