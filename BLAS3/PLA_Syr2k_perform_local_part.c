/*
   PLAPACK Release 3.0
   
   Copyright (C) 2000
   Robert A. van de Geijn and The University of Texas at Austin

   For GNU-license details see the file GNU_license in the 
   PLAPACK root directory.
*/

#include "PLA.h"

int PLA_Syr2k_perform_local_part( int uplo, 
				  PLA_Obj alpha, PLA_Obj Xdpmv, 
				                 PLA_Obj Xdpmv_trans,
				                 PLA_Obj Ydpmv, 
				                 PLA_Obj Ydpmv_trans,
                                  PLA_Obj beta,  PLA_Obj A )
{
  int 
    length, local_size, length_1,
    count, me;

  PLA_Obj 
    A11           = NULL,       A12           = NULL,
    A21           = NULL,       A22           = NULL,
    Xdpmv_1       = NULL,       Xdpmv_2       = NULL,
    Xdpmv_trans_1 = NULL,       Xdpmv_trans_2 = NULL,
    Ydpmv_1       = NULL,       Ydpmv_2       = NULL,
    Ydpmv_trans_1 = NULL,       Ydpmv_trans_2 = NULL,
    one = NULL;

  PLA_Create_constants_conf_to( A, NULL, NULL, &one );

  PLA_Obj_local_width( A, &local_size );
  if ( 0 == local_size ) return( PLA_FAILURE );
  PLA_Obj_local_length( A, &local_size );
  if ( 0 == local_size ) return( PLA_FAILURE );

  PLA_Obj_global_length( A, &length );

  if ( length >= 256 ) {
    length_1 = (length/2);
    length_1 = length_1 - length_1 % 64 + 64; 
    if ( length_1 >= length ) length = length/2;

    if ( uplo == PLA_LOWER_TRIANGULAR )
      PLA_Obj_split_4 ( A, length_1, length_1,      &A11, PLA_DUMMY,
		                                    &A21, &A22 );
    else
      PLA_Obj_split_4 ( A, length_1, length_1,      &A11,      &A12,
		                                    PLA_DUMMY, &A22 );

    PLA_Obj_horz_split_2( Xdpmv, length_1,        &Xdpmv_1,
                                                  &Xdpmv_2 );
    PLA_Obj_vert_split_2( Xdpmv_trans, length_1,  
                                  &Xdpmv_trans_1, &Xdpmv_trans_2 );

    PLA_Obj_horz_split_2( Ydpmv, length_1,        &Ydpmv_1,
                                                  &Ydpmv_2 );
    PLA_Obj_vert_split_2( Ydpmv_trans, length_1,  
			  &Ydpmv_trans_1, &Ydpmv_trans_2 );

    if ( uplo == PLA_LOWER_TRIANGULAR ){
      PLA_Local_gemm( PLA_NO_TRANS, PLA_NO_TRANS, 
	  	      alpha, Xdpmv_2, Ydpmv_trans_1, beta, A21 );
      PLA_Local_gemm( PLA_NO_TRANS, PLA_NO_TRANS, 
	  	      alpha, Ydpmv_2, Xdpmv_trans_1, one, A21 );
    }
    else{
      PLA_Local_gemm( PLA_NO_TRANS, PLA_NO_TRANS, 
	  	      alpha, Xdpmv_1, Xdpmv_trans_2, beta, A12 );
      PLA_Local_gemm( PLA_NO_TRANS, PLA_NO_TRANS, 
	  	      alpha, Ydpmv_1, Ydpmv_trans_2, one, A12 );
    }

    PLA_Syr2k_perform_local_part( uplo,
                                  alpha, Xdpmv_1, Xdpmv_trans_1,
				         Ydpmv_1, Ydpmv_trans_1,
                                  beta, A11 );

    PLA_Syr2k_perform_local_part( uplo,
                                 alpha, Xdpmv_2, Xdpmv_trans_2,
                                        Ydpmv_2, Ydpmv_trans_2,
                                 beta, A22 );
  } 
  else {    
    PLA_Syr2k_perform_local_part_by_panels( 
                                 uplo, alpha, Xdpmv, Xdpmv_trans,
				              Ydpmv, Ydpmv_trans,
				 beta,  A );
  }
  PLA_Obj_free( &A11 );                PLA_Obj_free( &A12 );
  PLA_Obj_free( &A21 );                PLA_Obj_free( &A22 );
  PLA_Obj_free( &Xdpmv_1 );            PLA_Obj_free( &Xdpmv_2 );
  PLA_Obj_free( &Xdpmv_trans_1 );      PLA_Obj_free( &Xdpmv_trans_2 );
  PLA_Obj_free( &Ydpmv_1 );            PLA_Obj_free( &Ydpmv_2 );
  PLA_Obj_free( &Ydpmv_trans_1 );      PLA_Obj_free( &Ydpmv_trans_2 );
  PLA_Obj_free( &one );

  return( PLA_SUCCESS );
}


int PLA_Syr2k_perform_local_part_by_panels( 
                                 int uplo, 
                                 PLA_Obj alpha, PLA_Obj Xdpmv, 
                                                 PLA_Obj Xdpmv_trans,
				                 PLA_Obj Ydpmv, 
                                                 PLA_Obj Ydpmv_trans,
                                 PLA_Obj beta,  PLA_Obj A )
{
  int 
    size, size_left, size_top, owner_top, owner_left, myrow, mycol, local_size;

  PLA_Obj 
    ABR    = NULL,   A12     = NULL,
    A11    = NULL,   A21     = NULL,
    Xdpmv_cur = NULL, Xdpmv_top = NULL,
    Xdpmv_trans_cur = NULL, Xdpmv_trans_left = NULL,
    Ydpmv_cur = NULL, Ydpmv_top = NULL,
    Ydpmv_trans_cur = NULL, Ydpmv_trans_left = NULL,
    one = NULL;

  PLA_Template 
    templ = NULL;

  PLA_Create_constants_conf_to( A, NULL, NULL, &one );

  PLA_Obj_local_width( A, &local_size );
  if ( local_size == 0 ) return( PLA_FAILURE );
  PLA_Obj_local_length( A, &local_size );
  if ( local_size == 0 ) return( PLA_FAILURE );

  PLA_Obj_view_all( A, &ABR );
  PLA_Obj_view_all( Xdpmv, &Xdpmv_cur );
  PLA_Obj_view_all( Xdpmv_trans, &Xdpmv_trans_cur );
  PLA_Obj_view_all( Ydpmv, &Ydpmv_cur );
  PLA_Obj_view_all( Ydpmv_trans, &Ydpmv_trans_cur );

  PLA_Obj_template( A, &templ );
  PLA_Temp_comm_row_rank( templ, &mycol );
  PLA_Temp_comm_col_rank( templ, &myrow );

  while ( TRUE ) {
    PLA_Obj_split_size ( ABR, PLA_SIDE_TOP,  &size_top,  &owner_top );
    PLA_Obj_split_size ( ABR, PLA_SIDE_LEFT, &size_left, &owner_left );

    if ( ( size = min ( size_top, size_left ) ) == 0 ) break;

    if ( uplo == PLA_LOWER_TRIANGULAR )
      PLA_Obj_split_4( ABR, size, size,     &A11, PLA_DUMMY,
		                            &A21, &ABR );
    else
      PLA_Obj_split_4( ABR, size, size,     &A11,      &A12,
		                            PLA_DUMMY, &ABR );

    PLA_Obj_vert_split_2( Xdpmv_trans_cur, size, 
                                 &Xdpmv_trans_left, &Xdpmv_trans_cur );

    PLA_Obj_horz_split_2( Xdpmv_cur, size,   &Xdpmv_top,
                                             &Xdpmv_cur );

    PLA_Obj_vert_split_2( Ydpmv_trans_cur, size, 
                                 &Ydpmv_trans_left, &Ydpmv_trans_cur );

    PLA_Obj_horz_split_2( Ydpmv_cur, size,   &Ydpmv_top,
                                             &Ydpmv_cur );

    if ( uplo == PLA_LOWER_TRIANGULAR ){
      if ( owner_left == mycol ) {
	PLA_Local_syr2k ( PLA_LOWER_TRIANGULAR, PLA_NO_TRANS,
			 alpha, Xdpmv_top, Ydpmv_top, beta, A11 );

	PLA_Local_gemm ( PLA_NO_TRANS, PLA_NO_TRANS,
			 alpha, Xdpmv_cur, Ydpmv_trans_left, beta, A21 );

	PLA_Local_gemm ( PLA_NO_TRANS, PLA_NO_TRANS,
			 alpha, Ydpmv_cur, Xdpmv_trans_left, one, A21 );
      }
    }
    else{
      if ( owner_left == mycol ) {
	PLA_Local_syr2k ( uplo, PLA_NO_TRANS,
			  alpha, Xdpmv_top, Ydpmv_top, beta, A11 );
      }
      if ( owner_top == myrow ) {
	PLA_Local_gemm ( PLA_NO_TRANS, PLA_NO_TRANS,
			 alpha, Xdpmv_top, Ydpmv_trans_cur, beta, A12 );

	PLA_Local_gemm ( PLA_NO_TRANS, PLA_NO_TRANS,
			 alpha, Ydpmv_top, Xdpmv_trans_cur, one, A12 );
      }
    }
  }

  PLA_Obj_free( &ABR );
  PLA_Obj_free( &A11 );
  PLA_Obj_free( &A12 );
  PLA_Obj_free( &A21 );
  PLA_Obj_free( &Xdpmv_cur );
  PLA_Obj_free( &Xdpmv_top );
  PLA_Obj_free( &Xdpmv_trans_cur );
  PLA_Obj_free( &Xdpmv_trans_left );
  PLA_Obj_free( &Ydpmv_cur );
  PLA_Obj_free( &Ydpmv_top );
  PLA_Obj_free( &Ydpmv_trans_cur );
  PLA_Obj_free( &Ydpmv_trans_left );
  PLA_Obj_free( &one );

  return( PLA_SUCCESS );
}






