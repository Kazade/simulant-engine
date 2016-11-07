#ifndef PIPELINE_HELPER_H
#define PIPELINE_HELPER_H

#include "viewport.h"
#include "types.h"

namespace smlt {

class PipelineHelper {
public:
    PipelineHelper to_framebuffer(const Viewport& view=Viewport());
    PipelineHelper to_texture(TextureID tex, const Viewport& view=Viewport());
    PipelineHelper with_priority(smlt::RenderPriority priority);
    PipelineHelper with_clear(uint32_t viewport_clear_flags=BUFFER_CLEAR_ALL);

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
    virtual PipelineHelper render(OverlayID, CameraID) = 0;

    virtual PipelinePtr pipeline(PipelineID pid) = 0;
    virtual bool enable_pipeline(PipelineID pid) = 0;
    virtual bool disable_pipeline(PipelineID pid) = 0;
    virtual void delete_pipeline(PipelineID pid) = 0;
    virtual bool has_pipeline(PipelineID pid) const = 0;
    virtual bool is_pipeline_enabled(PipelineID pid) const = 0;

protected:
    PipelineHelper new_pipeline_helper(std::shared_ptr<RenderSequence> sequence, StageID stage, CameraID cam);
    PipelineHelper new_pipeline_helper(std::shared_ptr<RenderSequence> sequence, OverlayID stage, CameraID cam);
};

}

#endif // PIPELINE_HELPER_H
