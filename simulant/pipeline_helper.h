/* *   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
 *
 *     This file is part of Simulant.
 *
 *     Simulant is free software: you can redistribute it and/or modify
 *     it under the terms of the GNU Lesser General Public License as published by
 *     the Free Software Foundation, either version 3 of the License, or
 *     (at your option) any later version.
 *
 *     Simulant is distributed in the hope that it will be useful,
 *     but WITHOUT ANY WARRANTY; without even the implied warranty of
 *     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *     GNU Lesser General Public License for more details.
 *
 *     You should have received a copy of the GNU Lesser General Public License
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
    PipelineHelper& operator=(const PipelineHelper& rhs) = default;

    operator PipelinePtr() const {
        return pipeline_;
    }

    /* Explicit conversion */
    PipelinePtr as_pipeline() const {
        return pipeline_;
    }

    PipelineHelper set_name(const std::string& name);

private:
    friend class PipelineHelperAPIInterface;

    typedef std::shared_ptr<RenderSequence> RenderSequencePtr;

    PipelineHelper(RenderSequencePtr sequence, PipelinePtr pid):
        sequence_(sequence), pipeline_(pid) {}

    RenderSequencePtr sequence_;
    PipelinePtr pipeline_;
};

class PipelineHelperAPIInterface {
public:
    virtual ~PipelineHelperAPIInterface() {}

    virtual PipelineHelper render(StageID, CameraID) = 0;

    virtual bool enable_pipeline(const std::string& name) = 0;
    virtual bool disable_pipeline(const std::string& name) = 0;
    virtual void destroy_pipeline(const std::string& name) = 0;
    virtual bool has_pipeline(const std::string& name) const = 0;
    virtual bool is_pipeline_active(const std::string& name) const = 0;

    virtual PipelinePtr find_pipeline(const std::string& name) = 0;

protected:
    PipelineHelper new_pipeline_helper(
        std::shared_ptr<RenderSequence> sequence, const std::string& name,
        StageID stage, CameraID cam
    );
};

}

#endif // PIPELINE_HELPER_H
