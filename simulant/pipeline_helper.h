/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU General Public License for more details.
 *
 *     You should have received a copy of the GNU General Public License
 *     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
 */

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
    PipelineHelper with_clear(
        uint32_t viewport_clear_flags=BUFFER_CLEAR_ALL,
        const smlt::Colour& clear_colour=Colour::GREY
    );

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
