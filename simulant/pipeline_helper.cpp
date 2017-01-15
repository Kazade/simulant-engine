//
//   Copyright (c) 2011-2017 Luke Benstead https://simulant-engine.appspot.com
//
//     This file is part of Simulant.
//
//     Simulant is free software: you can redistribute it and/or modify
//     it under the terms of the GNU General Public License as published by
//     the Free Software Foundation, either version 3 of the License, or
//     (at your option) any later version.
//
//     Simulant is distributed in the hope that it will be useful,
//     but WITHOUT ANY WARRANTY; without even the implied warranty of
//     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//     GNU General Public License for more details.
//
//     You should have received a copy of the GNU General Public License
//     along with Simulant.  If not, see <http://www.gnu.org/licenses/>.
//

#include "pipeline_helper.h"
#include "render_sequence.h"

namespace smlt {

PipelineHelper PipelineHelper::to_framebuffer(const Viewport &view) {
    sequence_->pipeline(pipeline_id_)->set_viewport(view);
    sequence_->pipeline(pipeline_id_)->set_target(TextureID());
    return *this;
}

PipelineHelper PipelineHelper::to_texture(TextureID tex, const Viewport& view) {
    sequence_->pipeline(pipeline_id_)->set_target(tex);
    sequence_->pipeline(pipeline_id_)->set_viewport(view);
    return *this;
}

PipelineHelper PipelineHelper::with_clear(uint32_t viewport_clear_flags, const smlt::Colour& clear_colour) {
    sequence_->pipeline(pipeline_id_)->set_clear_flags(viewport_clear_flags);
    sequence_->pipeline(pipeline_id_)->viewport->set_colour(clear_colour);
    return *this;
}

PipelineHelper PipelineHelper::with_priority(smlt::RenderPriority priority) {
    sequence_->pipeline(pipeline_id_)->set_priority((int) priority);
    return *this;
}


PipelineHelper PipelineHelperAPIInterface::new_pipeline_helper(RenderSequence::ptr sequence, StageID stage, CameraID cam) {
    PipelineID pid = sequence->new_pipeline(stage, cam);
    return PipelineHelper(sequence, pid);
}


}
