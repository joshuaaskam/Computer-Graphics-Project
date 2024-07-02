//
// Created by Joshua Askam on 6/27/24.
//

#pragma once
#include "Object3D.h"
#include "Animation.h"
/**
 * @brief Rotates an object at a continuous rate over an interval.
 */
class QuadraticBezierAnimation : public Animation {
private:
    /**
     * @brief How much to increment the translation by each second.
     */
    glm::vec3 p0;
    glm::vec3 p1;
    glm::vec3 p2;


    /**
     * @brief Advance the animation by the given time interval.
     */
    void applyAnimation(float dt) override {
        float t = currentTime()/duration();
        float bx = (1-t) * ((1 - t) * p0.x + t * p1.x) + t * ((1 - t) * p1.x + t * p2.x);
        float by = (1-t) * ((1 - t) * p0.y + t * p1.y) + t * ((1 - t) * p1.y + t * p2.y);
        float bz = (1-t) * ((1 - t) * p0.z + t * p1.z) + t * ((1 - t) * p1.z + t * p2.z);


        object().setPosition(glm::vec3 (bx, by, bz));
    }

public:
    /**
     * @brief Constructs a animation of a constant rotation by the given total rotation
     * angle, linearly interpolated across the given duration.
     */
    QuadraticBezierAnimation(Object3D& object, float duration, const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2 ) :
            Animation(object, duration), p0(p0), p1(p1), p2(p2) {}
};

