#include "plot/common.h"

// plot routine "glue" (rocket trail expanding between two vertices)
void
sx_plot_glue(uint32_t ei)
{
  // TODO: generate model->world matrix and hand that out to push instance!
}

int
sx_plot_glue_collide(const sx_entity_t *e, sx_obb_t *box, sx_part_type_t *pt)
{
  return 0;
}
