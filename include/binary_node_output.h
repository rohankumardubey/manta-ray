#ifndef MANTARAY_BINARY_NODE_OUTPUT_H
#define MANTARAY_BINARY_NODE_OUTPUT_H

#include "vector_node_output.h"

namespace manta {

    enum BINARY_OPERATION {
        ADD,
        SUB,
        DIV,
        MUL,
        DOT,
        CROSS,
        POW,
        MAX,
        MIN
    };

    template <BINARY_OPERATION op>
    class BinaryNodeOutput : public VectorNodeOutput {
    public:

    public:
        BinaryNodeOutput() {
            m_left = nullptr;
            m_right = nullptr;
        }

        virtual ~BinaryNodeOutput() {
            /* void */
        }

        static inline math::Vector doOp(const math::Vector &left, const math::Vector &right) {
            switch (op) {
            case ADD:
                return math::add(left, right);
            case SUB:
                return math::sub(left, right);
            case DIV:
                return math::div(left, right);
            case MUL:
                return math::mul(left, right);
            case DOT:
                return math::dot(left, right);
            case CROSS:
                return math::cross(left, right);
            case POW:
                return math::pow(left, right);
            case MAX:
                return math::componentMax(left, right);
            case MIN:
                return math::componentMin(left, right);
            default:
                return math::constants::Zero;
            }
        }

        virtual void sample(const IntersectionPoint *surfaceInteraction, void *_target) const {
            math::Vector *target = reinterpret_cast<math::Vector *>(_target);

            math::Vector left, right;
            static_cast<VectorNodeOutput *>(m_left)->sample(surfaceInteraction, (void *)&left);
            static_cast<VectorNodeOutput *>(m_right)->sample(surfaceInteraction, (void *)&right);

            *target = doOp(left, right);
        }

        virtual void discreteSample2d(int x, int y, void *_target) const {
            math::Vector *target = reinterpret_cast<math::Vector *>(_target);

            math::Vector left, right;
            static_cast<VectorNodeOutput *>(m_left)
                ->discreteSample2d(x, y, (void *)&left);
            static_cast<VectorNodeOutput *>(m_right)
                ->discreteSample2d(x, y, (void *)&right);

            *target = doOp(left, right);
        }

        virtual void getDataReference(const void **target) const {
            *target = nullptr;
        }

        virtual void _evaluateDimensions() {
            VectorNodeOutput *left = static_cast<VectorNodeOutput *>(m_left);
            VectorNodeOutput *right = static_cast<VectorNodeOutput *>(m_right);

            left->evaluateDimensions();
            right->evaluateDimensions();

            int dimensions = 0;
            if (left->getDimensions() > dimensions) dimensions = left->getDimensions();
            if (right->getDimensions() > dimensions) dimensions = right->getDimensions();

            setDimensions(dimensions);

            for (int i = 0; i < dimensions; i++) {
                int size = 0;
                if (left->getDimensions() > i &&  left->getSize(i) > size) size = left->getSize(i);
                if (right->getDimensions() > i && right->getSize(i) > size) size = right->getSize(i);

                setDimensionSize(i, size);
            }
        }

        piranha::pNodeInput *getLeftConnection() { return &m_left; }
        piranha::pNodeInput *getRightConnection() { return &m_right; }

    protected:
        virtual void registerInputs() {
            registerInput(&m_left);
            registerInput(&m_right);
        }

    protected:
        piranha::pNodeInput m_left;
        piranha::pNodeInput m_right;
    };

} /* namespace manta */

#endif /* MANTARAY_BINARY_NODE_OUTPUT_H */
