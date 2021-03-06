#ifndef _DOME_
#define _DOME_

#include "sgct.h"

enum{ STEP = 0, STEP_INVERTED, SOLID, GRADIENT, GRADIENT_INVERTED };

struct DomeVertex
{
    float x, y, z, s, t;
};

class Dome
{
public:
    Dome(float radius, float tilt);
    ~Dome();
    void drawGeoCorrPattern();
    void drawChannelZones();
    void drawBlendZones();
    void drawTexturedSphere();
    void drawColCorrPattern(glm::vec3 * color, int mode);
    void generateDisplayList();
    inline float getRadius(){ return mRadius; }
    inline void setTiltOffset(float diff) { mTiltOffset = diff; } 

private:
    void drawLatitudeLines(float latitude, float minLongitude, float maxLongitude, int segments);
    void drawLongitudeLines(float longitude, float minLatitude, float maxLatitude, int segments);
    void drawVerticalFrustumLine(glm::vec3 v1, glm::vec3 v2, int segments);
    void drawHorizontalFrustumLine(float verAngle, float minHorAngle, float maxHorAngle, int segments);

    float mRadius;
    float mTilt;
    float mTiltOffset;

    unsigned int mGeoDisplayList;
    unsigned int mBlendZonesDisplayList;
    unsigned int mChannelZonesDisplayList;
    unsigned int mTexDisplayList;

    std::vector<std::vector<DomeVertex>> mVertices;
    //std::vector<DomeVertex> mCapVertices;
};

#endif