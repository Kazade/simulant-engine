#ifndef PIPELINE_HELPER_H
#define PIPELINE_HELPER_H

#include "types.h"

namespace kglt {

class PipelineHelper {
public:
    PipelineHelper to_framebuffer(ViewportID view=ViewportID());
    PipelineHelper to_texture(TextureID tex, ViewportID view=ViewportID());
    PipelineHelper with_priority(kglt::RenderPriority priority);

    PipelineHelper(const PipelineHelper&) = default;
    PipelineHelper& operator=(PipelineHelper& rhs) = default;

    operator PipelineID() const {
        return pipeline_id_;
    }

private:
    friend class PipelineHelperAPIInterface;

    typedef std::shared_ptr<RenderSequence> RenderSequencePtr;

    PipelineHelper(RenderSequencePtr sequence, PipelineID pid):
        sequence_(sequence), pipeline_id_(pid) {}

    RenderSequencePtr sequence_;
    PipelineID pipeline_id_;
};

class PipelineHelperAPIInterface {
public:
    virtual ~PipelineHelperAPIInterface() {}

    virtual PipelineHelper render(StageID, CameraID) = 0;
    virtual PipelineHelper render(UIStageID, CameraID) = 0;

    virtual PipelinePtr pipeline(PipelineID pid) = 0;
    virtual bool enable_pipeline(PipelineID pid) = 0;
    virtual bool disable_pipeline(PipelineID pid) = 0;
    virtual void delete_pipeline(PipelineID pid) = 0;
    virtual bool has_pipeline(PipelineID pid) const = 0;
    virtual bool is_pipeline_enabled(PipelineID pid) const = 0;

protected:
    PipelineHelper new_pipeline_helper(std::shared_ptr<RenderSequence> sequence, StageID stage, CameraID cam);
    PipelineHelper new_pipeline_helper(std::shared_ptr<RenderSequence> sequence, UIStageID stage, CameraID cam);
};

}

#endif // PIPELINE_HELPER_H
