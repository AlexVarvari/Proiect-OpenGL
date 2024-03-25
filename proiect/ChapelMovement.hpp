// ChapelMovement.hpp
#include <glm/glm.hpp>

namespace gps {

    enum CHAPEL_MOVE_DIRECTION { CHAPEL_MOVE_FORWARD, CHAPEL_MOVE_BACKWARD, CHAPEL_MOVE_RIGHT, CHAPEL_MOVE_LEFT };


    class ChapelMovement {
    public:
        glm::vec3 chapelPosition;
        float angle;
        bool allowForward, allowBackward, allowLeft, allowRight;

        ChapelMovement() : chapelPosition(0.0f, 0.0f, 0.0f), angle(0.0f),
            allowForward(true), allowBackward(true), allowLeft(true), allowRight(true) {}

        void move(CHAPEL_MOVE_DIRECTION direction, float speed) {
            switch (direction) {
            case CHAPEL_MOVE_FORWARD:
                if (allowForward) {
                    chapelPosition += glm::vec3(sin(angle), 0.0f, cos(angle)) * speed;
                }
                break;
            case CHAPEL_MOVE_BACKWARD:
                if (allowBackward) {
                    chapelPosition -= glm::vec3(sin(angle), 0.0f, cos(angle)) * speed;
                }
                break;
            case CHAPEL_MOVE_RIGHT:
                if (allowRight) {
                    angle -= 0.01f;
                }
                break;
            case CHAPEL_MOVE_LEFT:
                if (allowLeft) {
                    angle += 0.01f;
                }
                break;
            }
        }
    };

}
