/*############################################################################*/
/*#                                                                          #*/
/*#  Ambisonic C++ Library                                                   #*/
/*#  Copyright © 2007 Aristotel Digenis                                      #*/
/*#                                                                          #*/
/*#  Filename:      AmbisonicCommons.cpp                                     #*/
/*#  Version:       0.1                                                      #*/
/*#  Date:          19/05/2007                                               #*/
/*#  Author(s):     Aristotel Digenis                                        #*/
/*#  Licence:       MIT                                                      #*/
/*#                                                                          #*/
/*############################################################################*/


#include "AmbisonicCommons.h"

float DegreesToRadians(float fDegrees)
{
    return fDegrees * (float)M_PI / 180.f;
}

float RadiansToDegrees(float fRadians)
{
    return fRadians * 180.f / (float)M_PI;
}

unsigned OrderToComponents(unsigned nOrder, bool b3D)
{
    if(b3D)
        return (unsigned) powf(nOrder + 1.f, 2.f);
    else
        return nOrder * 2 + 1;
}

unsigned OrderToComponentPosition(unsigned nOrder, bool b3D)
{


    unsigned nIndex = 0;

    if(b3D)
    {
        switch(nOrder)
        {
        case 0:    nIndex = 0;    break;
        case 1:    nIndex = 1;    break;
        case 2:    nIndex = 4;    break;
        case 3:    nIndex = 10;break;
        }
    }
    else
    {
        switch(nOrder)
        {
        case 0:    nIndex = 0;    break;
        case 1:    nIndex = 1;    break;
        case 2:    nIndex = 3;    break;
        case 3:    nIndex = 5;    break;
        }
    }

    return nIndex;
}

unsigned OrderToSpeakers(unsigned nOrder, bool b3D)
{

    if(b3D)
        return (nOrder * 2 + 2) * 2;
    else
        return nOrder * 2 + 2;
}

char ComponentToChannelLabel(unsigned nComponent, bool b3D)
{

    char cLabel = ' ';
    if(b3D)
    {
        switch(nComponent)
        {
        case 0:     cLabel = 'W';   break;
        case 1:     cLabel = 'Y';   break;
        case 2:     cLabel = 'Z';   break;
        case 3:     cLabel = 'X';   break;
        case 4:     cLabel = 'V';   break;
        case 5:     cLabel = 'T';   break;
        case 6:     cLabel = 'R';   break;
        case 7:     cLabel = 'U';   break;
        case 8:     cLabel = 'S';   break;
        case 9:     cLabel = 'Q';   break;
        case 10:    cLabel = 'O';   break;
        case 11:    cLabel = 'M';   break;
        case 12:    cLabel = 'K';   break;
        case 13:    cLabel = 'L';   break;
        case 14:    cLabel = 'N';   break;
        case 15:    cLabel = 'P';   break;
        };
    }
    else
    {
        switch(nComponent)
        {
        case 0:     cLabel = 'W';   break;
        case 1:     cLabel = 'X';   break;
        case 2:     cLabel = 'Y';   break;
        case 3:     cLabel = 'U';   break;
        case 4:     cLabel = 'V';   break;
        case 5:     cLabel = 'P';   break;
        case 6:     cLabel = 'Q';   break;
        };
    }

    return cLabel;
}

unsigned ComponentPositionToOrder(unsigned nComponent, bool b3D)
{
    if (b3D)
        return (unsigned)floorf(sqrtf((float)nComponent));
    else
        return (unsigned)floorf((nComponent + 1.f) * 0.5f);
}

unsigned OrderAndDegreeToComponent(int order, int degree, bool b3D)
{
    if (b3D)
        return order * (order + 1) + degree;
    else
        return degree < 0 ? 2 * order - 1 : 2 * order;
}

void ComponentToOrderAndDegree(int nComponent, bool b3D, int& order, int& degree)
{
    order = ComponentPositionToOrder(nComponent, b3D);

    if (b3D)
        degree = nComponent - order * (order + 1);
    else
        degree = nComponent % 2 == 0 ? order : -order;
}

template<typename T>
T N3dToSn3dFactor(int order)
{
    return static_cast<T>(1.) / (T)std::sqrt(static_cast<T>(2 * order) + 1.);
}
template float N3dToSn3dFactor(int order);
template double N3dToSn3dFactor(int order);

template<typename T>
T Sn3dToN3dFactor(int order)
{
    return (T)std::sqrt(static_cast<T>(2 * order) + 1.);
}
template float Sn3dToN3dFactor(int order);
template double Sn3dToN3dFactor(int order);

template<typename T>
T FuMaToSn3dFactor(int order, int degree)
{
    auto iComponent = OrderAndDegreeToComponent(order, degree, true);

    if (iComponent == 0)
        return std::sqrt(static_cast<T>(2));
    else if (iComponent < 4)
        return std::sqrt(static_cast<T>(2));
    else if (iComponent == 4 || iComponent == 5 || iComponent == 7 || iComponent == 8)
        return static_cast<T>(std::sqrt(3.) / 2.);
    else if (iComponent == 6)
        return static_cast<T>(1.);
    else if (iComponent == 9 || iComponent == 15)
        return static_cast<T>(std::sqrt(5. / 8.));
    else if (iComponent == 10 || iComponent == 14)
        return static_cast<T>(std::sqrt(5.) / 3.);
    else if (iComponent == 11 || iComponent == 13)
        return static_cast<T>(std::sqrt(32. / 45.));
    else if (iComponent == 12)
        return static_cast<T>(1.);

    return static_cast<T>(0);
}
template float FuMaToSn3dFactor(int order, int degree);
template double FuMaToSn3dFactor(int order, int degree);
