//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU Lesser General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU Lesser General Public License for more details.
//
//     You should have received a copy of the GNU Lesser General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "pipeline_helper.h"
#include "render_sequence.h"

namespace smlt {

PipelineHelper PipelineHelper::to_framebuffer(const Viewport &view) {
    pipeline_->set_viewport(view);
    pipeline_->set_target(TextureID());
    return *this;
}

PipelineHelper PipelineHelper::to_texture(TextureID tex, const Viewport& view) {
    pipeline_->set_target(tex);
    pipeline_->set_viewport(view);
    return *this;
}

PipelineHelper PipelineHelper::with_clear(uint32_t viewport_clear_flags, const smlt::Colour& clear_colour) {
    pipeline_->set_clear_flags(viewport_clear_flags);
    pipeline_->viewport->set_colour(clear_colour);
    return *this;
}

PipelineHelper PipelineHelper::set_name(const std::string& name) {
    pipeline_->set_name(name);
    return *this;
}

PipelineHelper PipelineHelper::with_priority(smlt::RenderPriority priority) {
    pipeline_->set_priority((int) priority);
    return *this;
}

PipelineHelper PipelineHelperAPIInterface::new_pipeline_helper(RenderSequence::ptr sequence, const std::string& name, StageID stage, CameraID cam) {
    PipelinePtr pid = sequence->new_pipeline(name, stage, cam);
    return PipelineHelper(sequence, pid);
}


}
