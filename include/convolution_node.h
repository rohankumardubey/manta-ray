#ifndef CONVOLUTION_NODE_H
#define CONVOLUTION_NODE_H

#include <node.h>

#include <vector_map_2d.h>
#include <vector_map_2d_node_output.h>

namespace manta {

	class ConvolutionNode : public Node {
	public:
		ConvolutionNode();
		~ConvolutionNode();

		void setInputs(const VectorMap2DNodeOutput *base, const VectorMap2DNodeOutput *filter) { m_base = base; m_filter = filter; }

		void setResize(bool resize) { m_resize = resize; }
		bool getResize() const { return m_resize; }

		void setClip(bool clip) { m_clip = clip; }
		bool getClip() const { return m_clip; }

		const VectorMap2DNodeOutput *getMainOutput() const { return &m_output; }

	protected:
		virtual void _initialize();
		virtual void _evaluate();
		virtual void _destroy();

		virtual void registerInputs();
		virtual void registerOutputs();

	protected:
		const VectorMap2DNodeOutput *m_base;
		const VectorMap2DNodeOutput *m_filter;
		VectorMap2DNodeOutput m_output;
		bool m_resize;
		bool m_clip;

		VectorMap2D m_outputMap;
	};

} /* namespace manta */

#endif /* IMAGE_OUTPUT_NODE_H */

