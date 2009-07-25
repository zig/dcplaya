/** Layer III frame side information structures.
 *
 *    This structure is not used. It is given for documentation only.
 *    The bitfield must be considered as packed across struct.
 */
typedef struct {
  union {

    struct {
      int res : 9;                 /**< Size of bit reservoir. */
      int private : 5;             /**< Number of private bits ? */
      int scfi0 : 4;               /**< ??? */
      struct {
	int p23_len : 12;
	int big_value : 9;
	int gain : 8;
	int scale_compress : 4;
	int window_flag : 1;

	union {
	  struct {
	    struct {
	      int value : 5;
	    } tab_select[3];
	    int region0 : 4;
	    int region1 : 3;
	  } window0;

	  struct {
	    int block_type : 2;
	    int mixed_block_flags : 1;
	    struct {
	      int value : 5;
	    } tab_select[2];
	    struct {
	      int value : 3;
	    } full_gain[3];
	  } window1;

	} window;

	int pre_flags : 1;
	int scalefactor_scale : 1;
	int scalefactor_scale : 1;
	int count1table_select : 1;
	
      } granules[2];

    } lsf0_chan0;

  } format;

} mpg123_III_side_info_t
