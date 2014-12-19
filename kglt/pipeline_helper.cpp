#include "pipeline_helper.h"
#include "render_sequence.h"

namespace kglt {

PipelineHelper PipelineHelper::to_framebuffer(ViewportID view) {
    sequence_->pipeline(pipeline_id_)->set_viewport(view);
    sequence_->pipeline(pipeline_id_)->set_target(TextureID());
    return *this;
}

PipelineHelper PipelineHelper::to_texture(TextureID tex, ViewportID view) {
    sequence_->pipeline(pipeline_id_)->set_target(tex);
    sequence_->pipeline(pipeline_id_)->set_viewport(view);
    return *this;
}

PipelineHelper PipelineHelper::with_priority(kglt::RenderPriority priority) {
    sequence_->pipeline(pipeline_id_)->set_priority((int) priority);
    return *this;
}


PipelineHelper PipelineHelperAPIInterface::new_pipeline_helper(RenderSequence::ptr sequence, StageID stage, CameraID cam) {
    PipelineID pid = sequence->new_pipeline(stage, cam);
    return PipelineHelper(sequence, pid);
}

PipelineHelper PipelineHelperAPIInterface::new_pipeline_helper(RenderSequence::ptr sequence, UIStageID stage, CameraID cam) {
    PipelineID pid = sequence->new_pipeline(stage, cam);
    return PipelineHelper(sequence, pid);
}


}
