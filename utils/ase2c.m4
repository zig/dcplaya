
obj_driver_t ASENAME =
{
  /* any_driver_t */
  {
    /* Next        */ NEXT_DRIVER,
    /* Type        */ OBJ_DRIVER,
    /* Version     */ 0x100,
    /* Object name */ "ASENAME",
    /* Authors     */ "Ben(jamin) Gerard\0",
    /* Description */ "Little 3D object",
    /* DLL         */ 0,
    /* Init        */ obj3d_init,
    /* Shutdown    */ obj3d_shutdown,
    /* Option      */ obj3d_options
  },

  /* obj_t */
  {
    /* Name          */  0,
    /* Flags         */  0,
    /* Num vtx       */  NBV,
    /* Num faces     */  NBF,
    /* Num vtx check */  sizeof(ASENAME`_vtx') / sizeof(*ASENAME`_vtx'),
    /* Num faces chk */  sizeof(ASENAME`_tri') / sizeof(*ASENAME`_tri'),
    /* Vertrices     */  ASENAME`_vtx',
    /* Triangles     */  ASENAME`_tri',
    /* Links         */  ASENAME`_tlk',
    /* Normal        */  0
  }
};

EXPORT_DRIVER(ASENAME)

