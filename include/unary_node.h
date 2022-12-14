#ifndef MANTARAY_UNARY_NODE_H
#define MANTARAY_UNARY_NODE_H

#include "node.h"

#include "unary_node_output.h"
#include "uv_operation_output.h"

namespace manta {

    template <typename OutputType>
    class SingleOutputNode : public Node {
    public:
        SingleOutputNode(const char *inputName = "__in", const char *outputName = "__out") {
            m_inputName = inputName;
            m_outputName = outputName;
        }

        virtual ~SingleOutputNode() {
            /* void */
        }

    protected:
        const char *m_inputName;
        const char *m_outputName;

        OutputType m_output;

        virtual void _initialize() {
            m_output.initialize();
        }

        virtual void registerInputs() {
            registerInput(m_output.getConnection(), m_inputName);
        }

        virtual void registerOutputs() {
            setPrimaryOutput(m_outputName);
            registerOutput(&m_output, m_outputName);
        }
    };

    template <UNARY_OPERATION op>
    using UnaryNode = SingleOutputNode<UnaryNodeOutput<op>>;

    template <UvOperation op>
    using UvNode = SingleOutputNode<UvOperationOutput<op>>;

    typedef UnaryNode<NEGATE> VectorNegateNode;
    typedef UnaryNode<NORMALIZE> VectorNormalizeNode;
    typedef UnaryNode<MAGNITUDE> VectorMagnitudeNode;
    typedef UnaryNode<MAX_COMPONENT> VectorMaxComponentNode;
    typedef UnaryNode<ABSOLUTE> VectorAbsoluteNode;
    typedef UnaryNode<SIN> VectorSinNode;

    typedef UvNode<UvOperation::Wrap> UvWrapNode;

} /* namespace manta */

#endif /* MANTARAY_UNARY_NODE_H */
