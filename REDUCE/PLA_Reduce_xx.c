/*
   PLAPACK Release 3.0
   
   Copyright (C) 2000
   Robert A. van de Geijn and The University of Texas at Austin

   For GNU-license details see the file GNU_license in the 
   PLAPACK root directory.
*/

#include "PLA.h"
#include "PLA_Reduce.h"

extern int report_case;

/*----------------------------------------------------------------------*/

int PLA_Reduce_xx( int shape, PLA_Obj Obj_from, MPI_Op op, 
                    PLA_Obj alpha, PLA_Obj Obj_to )

/************************************************************************
 
  Reduce contents of Obj_from to Obj_to
  Expert version: allows shape and adding to multiple of target

*************************************************************************/

{
  int objtype_from, proj_onto, owner_col, owner_row, length, width;

  /* Check for quick return */
  PLA_Obj_global_length( Obj_to, &length );
  PLA_Obj_global_width( Obj_to, &width );

  if ( length == 0 || width == 0 ) return( PLA_FAILURE );

  PLA_Obj_objtype( Obj_from, &objtype_from );

  switch( objtype_from ){
  case PLA_MSCALAR: 
    PLA_Reduce_xx_from_msc( shape, Obj_from, op, alpha, Obj_to );
    break;
/*  case PLA_VECTOR:   */
  case PLA_PMVECTOR: 
    PLA_Obj_project_onto( Obj_from, &proj_onto );
    switch( proj_onto ){
    case PLA_PROJ_ONTO_ROW: 
    case PLA_PROJ_ONTO_COL:
      PLA_Obj_owner_col( Obj_from, &owner_col );
      PLA_Obj_owner_row( Obj_from, &owner_row );
      if ( owner_col == PLA_ALL_COLS ||
		      owner_row == PLA_ALL_ROWS )
	PLA_Reduce_xx_from_dpmv( shape, Obj_from, op, alpha, Obj_to );
      else
	PLA_Reduce_xx_from_pmv( shape, Obj_from, op, alpha, Obj_to );
      break;
    }
    break;
  case PLA_MVECTOR: 
    PLA_Reduce_xx_from_mv( shape, Obj_from, op, alpha, Obj_to );
    break;
  case PLA_MATRIX:  
    PLA_Reduce_xx_from_matrix( shape, Obj_from, op, alpha,  Obj_to );
    break;
  default:
    printf("Obj_from in PLA_Reduce has unknown objtype\n");
    exit( 0 );
  }
  
  return( PLA_SUCCESS );
}

int PLA_Reduce_xx_from_matrix( int shape, PLA_Obj Obj_from, MPI_Op op, 
			        PLA_Obj alpha, PLA_Obj Obj_to )
{
  int objtype_to, proj_onto, owner_col, owner_row;

  return PLA_Reduce_old( Obj_from, op, Obj_to );
}


/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc( int shape, PLA_Obj Obj_from, MPI_Op op, 
			     PLA_Obj alpha, PLA_Obj Obj_to )

/************************************************************************
 
  Reduce from multiscalar

*************************************************************************/

{
  int objtype_to, proj_onto, owner_col, owner_row;

  PLA_Obj_objtype( Obj_to, &objtype_to );
  
  switch( objtype_to ){
  case PLA_MSCALAR: 
    PLA_Reduce_xx_from_msc_to_msc( shape, Obj_from, op, alpha,  Obj_to );
    break;
/*  case PLA_VECTOR:   */
  case PLA_PMVECTOR: 
    PLA_Obj_project_onto( Obj_to, &proj_onto );
    switch( proj_onto ){
    case PLA_PROJ_ONTO_ROW: 
    case PLA_PROJ_ONTO_COL:
      PLA_Obj_owner_col( Obj_to, &owner_col );
      PLA_Obj_owner_row( Obj_to, &owner_row );
      if ( owner_col == PLA_ALL_COLS ||
	  owner_row == PLA_ALL_ROWS )
	PLA_Reduce_xx_from_msc_to_dpmv( shape, Obj_from, op, alpha,  Obj_to );
      else
	PLA_Reduce_xx_from_msc_to_pmv( shape, Obj_from, op, alpha,  Obj_to );
      break;
    }
    break;
  case PLA_MVECTOR: 
    PLA_Reduce_xx_from_msc_to_mv( shape, Obj_from, op, alpha,  Obj_to );
    break;
  case PLA_MATRIX:  PLA_Reduce_xx_from_msc_to_matrix( shape, Obj_from, op, alpha,  Obj_to );
               break;
  default:     printf("Obj_to in PLA_Reduce has unknown objtype\n");
               exit( 0 );
  }

  return( PLA_SUCCESS );
}



/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc_to_dpmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				     PLA_Obj alpha, PLA_Obj Obj_to )

/************************************************************************
 
  Reduce from multiscalar to duplicated projected multivector

*************************************************************************/

{
  if ( report_case ) {
    printf("PLA_Copy_from_msc_to_dpmv not yet implemented\n");
    report_case = FALSE;
  }
  return PLA_Reduce_old( Obj_from, op, Obj_to );
}

/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc_to_matrix( int shape, PLA_Obj Obj_from, MPI_Op op, 
				       PLA_Obj alpha, PLA_Obj Obj_to )
{
/************************************************************************
 
  Reduce from multiscalar to matrix

*************************************************************************/


  if ( report_case ) {
    printf("PLA_Reduce_xx_from_msc_to_matrix not yet implemented\n");
    report_case = FALSE;
  }
  return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
}


/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc_to_msc( int shape, PLA_Obj Obj_from, MPI_Op op, 
				    PLA_Obj alpha, PLA_Obj Obj_to )

/************************************************************************
 
  Reduce from multiscalar to multiscalar

*************************************************************************/
  
{
  int
    done = FALSE,
    owner_row_from, owner_col_from,
    owner_row_to,   owner_col_to,
    length, width, ldim_from, ldim_to;
  void 
    *buffer_from, *buffer_to;
  MPI_Datatype
    datatype;
  MPI_Comm
    comm = MPI_COMM_NULL;
  PLA_Template
    templ;

  PLA_Obj_owner_row( Obj_from, &owner_row_from );
  PLA_Obj_owner_col( Obj_from, &owner_col_from );
  PLA_Obj_owner_row( Obj_to, &owner_row_to );
  PLA_Obj_owner_col( Obj_to, &owner_col_to );

  if ( owner_row_from == PLA_ALL_ROWS && 
       owner_col_from == PLA_ALL_COLS &&
       owner_row_to   == PLA_ALL_ROWS &&
       owner_col_to   == PLA_ALL_COLS ){
    PLA_Obj_template     ( Obj_from, &templ );
    PLA_Temp_comm_all     ( templ,    &comm ); 
    PLA_Obj_datatype     ( Obj_from, &datatype );
    PLA_Obj_global_length( Obj_from, &length );
    PLA_Obj_global_width ( Obj_from, &width );
    PLA_Obj_local_ldim   ( Obj_from, &ldim_from );
    PLA_Obj_local_ldim   ( Obj_to,   &ldim_to );
    PLA_Obj_local_buffer ( Obj_from, &buffer_from );
    PLA_Obj_local_buffer ( Obj_to,   &buffer_to );

    if ( shape == PLA_SHAPE_GENERAL && PLA_Local_equal_zero( alpha ) ){
      if ( length == ldim_from && length == ldim_to ){
	MPI_Allreduce( buffer_from, buffer_to, length*width, datatype,
  		       op, comm );
	done = TRUE;
      }
    }
  }      

  if ( !done ){
    if ( report_case ) {
      printf("PLA_Reduce_xx_from_msc_to_msc not yet implemented\n");
      report_case = FALSE;
    }

    return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
  }
}


/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc_to_mv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				   PLA_Obj alpha, PLA_Obj Obj_to )
{
/************************************************************************
 
  Reduce from multiscalar to multivector

*************************************************************************/


  if ( report_case ) {
    printf("PLA_Reduce_xx_from_msc_to_mv not yet implemented\n");
    report_case = FALSE;

    return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
  }
}


/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_msc_to_pmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				    PLA_Obj alpha, PLA_Obj Obj_to )
{
/************************************************************************
 
  Reduce from multiscalar to projected multivector

*************************************************************************/


  if ( report_case ) {
    printf("PLA_Reduce_xx_from_msc_to_ not yet implemented\n");
    report_case = FALSE;
  }
  return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
}

/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_mv( int shape, PLA_Obj Obj_from, MPI_Op op, 
			    PLA_Obj alpha, PLA_Obj Obj_to )
{
/************************************************************************
 
  Reduce from multivector

*************************************************************************/

  return PLA_Reduce_old( Obj_from, op, Obj_to );
}

/*----------------------------------------------------------------------*/

int PLA_Reduce_xx_from_pmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
			     PLA_Obj alpha, PLA_Obj Obj_to )
{
/************************************************************************
 
  Reduce from projected multivector

*************************************************************************/

  return PLA_Reduce_old( Obj_from, op, Obj_to );
}

int PLA_Reduce_xx_from_dpmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
			      PLA_Obj alpha, PLA_Obj Obj_to )
{
  int objtype_to, proj_onto, owner_col, owner_row;

  PLA_Obj_objtype( Obj_to, &objtype_to );
  
  switch( objtype_to ){
  case PLA_MSCALAR: 
    PLA_Reduce_xx_from_dpmv_to_msc( shape, Obj_from, op, alpha,  Obj_to );
    break;
/*  case PLA_VECTOR:  */
  case PLA_PMVECTOR: 
    PLA_Obj_project_onto( Obj_to, &proj_onto );
    switch( proj_onto ){
    case PLA_PROJ_ONTO_ROW: 
    case PLA_PROJ_ONTO_COL:
      PLA_Obj_owner_col( Obj_to, &owner_col );
      PLA_Obj_owner_row( Obj_to, &owner_row );
      if ( owner_col == PLA_ALL_COLS ||
	  owner_row == PLA_ALL_ROWS )
	PLA_Reduce_xx_from_dpmv_to_dpmv( shape, Obj_from, op, alpha,  Obj_to );
      else
	PLA_Reduce_xx_from_dpmv_to_pmv( shape, Obj_from, op, alpha,  Obj_to );
    }
    break;
  case PLA_MVECTOR: 
    PLA_Reduce_xx_from_dpmv_to_mv( shape, Obj_from, op, alpha,  Obj_to );
    break;
  case PLA_MATRIX:  
    PLA_Reduce_xx_from_dpmv_to_matrix( shape, Obj_from, op, alpha,  Obj_to );
    break;
  default:
    printf("Obj_to in PLA_Reduce has unknown objtype\n");
    exit( 0 );
  }
}

int PLA_Reduce_xx_from_dpmv_to_msc( int shape, PLA_Obj Obj_from, MPI_Op op, 
				     PLA_Obj alpha, PLA_Obj Obj_to )
{
  if ( report_case ) {
    printf("PLA_Reduce_xx_from_dpmv_to_msc not yet implemented\n");
    report_case = FALSE;
  }
  return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
}


int PLA_Reduce_xx_from_dpmv_to_matrix( int shape, PLA_Obj Obj_from, MPI_Op op, 
				        PLA_Obj alpha, PLA_Obj Obj_to )
{
  int 
    done = FALSE,
    size, proj_onto_from, proj_onto_to,
    local_length_from, local_length_to, 
    local_width_from, local_width_to, 
    local_ldim_from, local_ldim_to,
    align_row_from, align_row_to,
    align_col_from, align_col_to,
    owner, mycol, myrow, 
    totalsize, typesize;

  void 
    *local_buffer_from, *local_buffer_to, *buffer_to_temp, *buffer_from_temp;

  PLA_Obj
    Obj_temp = NULL,
    from_cur = NULL, to_cur = NULL,
    from_1   = NULL, to_1   = NULL,   to_1_temp = NULL;
  
  PLA_Template
    templ;

  MPI_Comm
    comm = MPI_COMM_NULL;

  MPI_Datatype
    datatype;

  PLA_Obj_project_onto( Obj_from, &proj_onto_from );
  PLA_Obj_project_onto( Obj_to,   &proj_onto_to );

  if ( proj_onto_from != proj_onto_to ) {
    if ( proj_onto_from == PLA_PROJ_ONTO_ROW )
      PLA_Obj_global_length( Obj_from, &size );
    else 
      PLA_Obj_global_width( Obj_from, &size );

    PLA_Mvector_create_conf_to( Obj_from, size, &Obj_temp );

    PLA_Reduce_xx_from_dpmv_to_mv( shape, Obj_from, op, alpha,  Obj_temp );

    PLA_Copy( Obj_temp, Obj_to );

    PLA_Obj_free( &Obj_temp );

    done = TRUE;
  }
  else {   /* proj_onto_from == proj_onto_to */
    PLA_Obj_datatype( Obj_from, &datatype );
    if ( proj_onto_from == PLA_PROJ_ONTO_COL ){
      PLA_Obj_global_align_row( Obj_from, &align_row_from );
      PLA_Obj_global_align_row( Obj_to,   &align_row_to );

      if ( align_row_from == align_row_to ){
	PLA_Obj_template( Obj_from, &templ );
	PLA_Temp_comm_row( templ, &comm );
	PLA_Temp_comm_row_rank( templ, &mycol );

	PLA_Obj_local_length( Obj_from, &local_length_from );
	PLA_Obj_local_ldim(   Obj_from, &local_ldim_from );
	PLA_Obj_local_buffer( Obj_from, &local_buffer_from);

	PLA_Obj_local_length( Obj_to,   &local_length_to );
	PLA_Obj_local_ldim(   Obj_to,   &local_ldim_to );
	PLA_Obj_local_buffer( Obj_to,   &local_buffer_to );
	
	PLA_Obj_view_all( Obj_from, &from_cur );
	PLA_Obj_view_all( Obj_to,   &to_cur );

	while( TRUE ){
	  PLA_Obj_split_size_to_next_proc( to_cur, PLA_SIDE_LEFT,
					    &size, &owner );
	  if ( size == 0 ) break;

	  PLA_Obj_vert_split_2( from_cur, size, &from_1, &from_cur );
	  PLA_Obj_vert_split_2( to_cur,   size, &to_1,   &to_cur );

	  MPI_Type_size ( datatype, &typesize );
	  totalsize = local_length_from * size;
	  if ( local_length_from != local_ldim_from ){
	    buffer_from_temp = PLA_malloc( (size_t) totalsize * typesize );
	    PLA_Obj_get_local_contents( from_1, PLA_NO_TRANS, 
					 &local_length_from, &size, 
					 buffer_from_temp, local_length_from,
					 1 );
	  }
	  else 
	    PLA_Obj_local_buffer( from_1, &buffer_from_temp );

	  if ( owner == mycol ){
	    if ( ( local_length_to != local_ldim_to ) ||
		( shape != PLA_SHAPE_GENERAL ) ||
		( !PLA_Local_equal_zero( alpha ) ) ){
	      PLA_Matrix_create_conf_to( to_1, &to_1_temp ); 
	      PLA_Obj_local_buffer( to_1_temp, &buffer_to_temp );
	    }
	    else {
	      PLA_Obj_local_buffer( to_1, &buffer_to_temp );
	    }
	  }

	  PLA_MPI_Reduce( BF( buffer_from_temp ), BF( buffer_to_temp ), 
		      totalsize, datatype, op, owner, comm );

	  if ( local_length_from != local_ldim_from )
	    PLA_free( buffer_from_temp );

	  if ( mycol == owner ){
	    if ( ( local_length_to != local_ldim_to ) ||
		( shape != PLA_SHAPE_GENERAL ) ||
		( !PLA_Local_equal_zero( alpha ) ) ){
	      PLA_Perform_local_part_of_reduce_xx( 
                     shape, to_1_temp, op, alpha, to_1 );
	    }
	  }
	}
	
	PLA_Obj_free( &to_cur );
	PLA_Obj_free( &to_1 );
	PLA_Obj_free( &to_1_temp );
	PLA_Obj_free( &from_cur );
	PLA_Obj_free( &from_1 );

	done = TRUE;
      }
    }
    else if ( proj_onto_from == PLA_PROJ_ONTO_ROW ){
      PLA_Obj_global_align_col( Obj_from, &align_col_from );
      PLA_Obj_global_align_col( Obj_to,   &align_col_to );

      if ( align_col_from == align_col_to ){
	PLA_Obj_template( Obj_from, &templ );
	PLA_Temp_comm_col( templ, &comm );
	PLA_Temp_comm_col_rank( templ, &myrow );

	PLA_Obj_local_width(  Obj_from, &local_width_from );
	PLA_Obj_local_ldim(   Obj_from, &local_ldim_from );
	PLA_Obj_local_buffer( Obj_from, &local_buffer_from);

	PLA_Obj_local_width(  Obj_to,   &local_width_to );
	PLA_Obj_local_ldim(   Obj_to,   &local_ldim_to );
	PLA_Obj_local_buffer( Obj_to,   &local_buffer_to );
	
	PLA_Obj_view_all( Obj_from, &from_cur );
	PLA_Obj_view_all( Obj_to,   &to_cur );

	while( TRUE ){
	  PLA_Obj_split_size( to_cur, PLA_SIDE_TOP, &size, &owner );
	  if ( size == 0 ) break;

	  PLA_Obj_horz_split_2( from_cur, size, &from_1, 
                                                 &from_cur );
	  PLA_Obj_horz_split_2( to_cur,   size, &to_1,   
                                                 &to_cur );

	  MPI_Type_size ( datatype, &typesize );
	  totalsize = local_width_from * size;
	  if ( size != local_ldim_from ){
	    buffer_from_temp = PLA_malloc( (size_t) totalsize * typesize );
	    PLA_Obj_get_local_contents( from_1, PLA_NO_TRANS, 
					 &size, &local_width_from, 
					 buffer_from_temp, size,
					 1 );
	  }
	  else 
	    PLA_Obj_local_buffer( from_1, &buffer_from_temp );

	  if ( owner == myrow ){
	    if ( ( size != local_ldim_to ) ||
		( shape != PLA_SHAPE_GENERAL ) ||
		( !PLA_Local_equal_zero( alpha ) ) ){
	      PLA_Matrix_create_conf_to( to_1, &to_1_temp ); 
	      PLA_Obj_local_buffer( to_1_temp, &buffer_to_temp );
	    }
	    else {
	      PLA_Obj_local_buffer( to_1, &buffer_to_temp );
	    }
	  }

	  PLA_MPI_Reduce( BF( buffer_from_temp ), BF( buffer_to_temp ), 
		      totalsize, datatype, op, owner, comm );

	  if ( size != local_ldim_from )
	    PLA_free( buffer_from_temp );

	  if ( myrow == owner ){
	    if ( ( size != local_ldim_to ) ||
		( shape != PLA_SHAPE_GENERAL ) ||
		( !PLA_Local_equal_zero( alpha ) ) ){
	      PLA_Perform_local_part_of_reduce_xx( 
                     shape, to_1_temp, op, alpha, to_1 );
	    }
	  }
	}

	PLA_Obj_free( &to_cur );
	PLA_Obj_free( &to_1 );
	PLA_Obj_free( &to_1_temp );
	PLA_Obj_free( &from_cur );
	PLA_Obj_free( &from_1 );

	done = TRUE;
      }
    }
  }
    
  if ( !done ) {
    if ( report_case ) {
      printf("PLA_Reduce_xx_from_dpmv_to_matrix case not yet implemented\n");
      report_case = FALSE;
    }
    return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
  }
  else return( PLA_SUCCESS );
}


int PLA_Reduce_xx_from_dpmv_to_mv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				    PLA_Obj alpha, PLA_Obj Obj_to )
{
  int 
    done = FALSE, proj_onto, size;
  PLA_Obj
    Obj_temp = NULL;
  
  PLA_Obj_project_onto( Obj_from, &proj_onto );
  PLA_Obj_global_width( Obj_to, &size );

  if ( proj_onto == PLA_PROJ_ONTO_COL )
    PLA_Pmvector_create_conf_to( Obj_from, PLA_PROJ_ONTO_COL, PLA_ALL_COLS, 
				 size, &Obj_temp );
  else
    PLA_Pmvector_create_conf_to( Obj_from, PLA_PROJ_ONTO_ROW, PLA_ALL_ROWS, 
				 size, &Obj_temp );

  PLA_Reduce_xx_from_dpmv_to_dpmv( shape, Obj_from, op, alpha,  Obj_temp );

  PLA_Copy_from_dpmv_to_mv( Obj_temp, Obj_to );

  PLA_Obj_free( &Obj_temp );
  done = TRUE;

  if ( !done ) {
    if ( report_case ) {
      printf("PLA_Reduce_xx_from_dpmv_to_mv not yet implemented\n");
      report_case = FALSE;
    }
    
    return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
  }
  else return( PLA_SUCCESS );
}


int PLA_Reduce_xx_from_dpmv_to_pmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				     PLA_Obj alpha, PLA_Obj Obj_to )
{
  if ( report_case ) {
    printf("PLA_Reduce_xx_from_dpmv_to_pmv not yet implemented\n");
    report_case = FALSE;
  }
  return PLA_Reduce_old( shape, Obj_from, op, alpha,  Obj_to );
}


int PLA_Reduce_xx_from_dpmv_to_dpmv( int shape, PLA_Obj Obj_from, MPI_Op op, 
				      PLA_Obj alpha, PLA_Obj Obj_to )
{
  int 
    done = FALSE, 
    proj_onto_from, proj_onto_to,
    local_length, local_width, 
    local_ldim_from, local_ldim_to,
    typesize;
  void
    *buffer_from, *buffer_to, *buffer_temp_from, *buffer_temp_to;
  MPI_Comm
    comm = MPI_COMM_NULL;
  MPI_Datatype 
    datatype;
  PLA_Template
    templ = NULL;

  PLA_Obj_project_onto( Obj_from, &proj_onto_from );
  PLA_Obj_project_onto( Obj_to, &proj_onto_to );

  switch( proj_onto_from ){
  case PLA_PROJ_ONTO_COL:                          /* Obj_from PROJ_ONTO_COL */
    {
      if ( proj_onto_to == proj_onto_from ) {  /* Both dpmv's proje onto col */
	int align_row_from, align_row_to;

	PLA_Obj_global_align_row( Obj_from, &align_row_from );
	PLA_Obj_global_align_row( Obj_to,   &align_row_to );
	if ( align_row_from == align_row_to ){ 	        /* Alignment matches */
	  PLA_Obj_datatype(     Obj_from, &datatype );
	  MPI_Type_size( datatype, &typesize );
	  PLA_Obj_local_length( Obj_from, &local_length );
	  PLA_Obj_local_width(  Obj_from, &local_width );
	  PLA_Obj_local_ldim(   Obj_from, &local_ldim_from );
	  PLA_Obj_local_ldim(   Obj_to,   &local_ldim_to );
	  PLA_Obj_local_buffer( Obj_from, &buffer_from );
	  PLA_Obj_local_buffer( Obj_to,   &buffer_to );
	  PLA_Obj_template(     Obj_from,   &templ );
	  PLA_Temp_comm_row ( templ, &comm );

	  if ( local_ldim_from == local_length ){
	    if ( local_ldim_to == local_length ){     /* both objects packed */
	      PLA_MPI_Allreduce( buffer_from, buffer_to, 
			     local_length * local_width, datatype, op, comm );
	    }
	    else  {                                     /* Obj_to not packed */
	      buffer_temp_to = 
		PLA_malloc( (size_t) typesize * local_length * local_width );

	      PLA_MPI_Allreduce( buffer_from, buffer_temp_to, 
			     local_length * local_width, datatype, op, comm );

	      pla_array_copy_new( datatype, local_length, local_width,
				  buffer_temp_to, local_length, buffer_to,
                                  local_ldim_to );

	      PLA_free( buffer_temp_to );
	    }
	  }                                           /* Obj_from not packed */
	  else  {
	    buffer_temp_from = 
	      PLA_malloc( (size_t) typesize * local_length * local_width );

	    PLA_Obj_get_local_contents( Obj_from, PLA_NO_TRANSPOSE, 
				        &local_length, &local_width, 
				        buffer_temp_from, 
				        local_length, 1 );

	    if ( local_ldim_to == local_length ){            /* Obj_to packed */
	      PLA_MPI_Allreduce( buffer_temp_from, buffer_to, 
			    local_length * local_width, 
			    datatype, op, comm );
	    }
	    else  {                                       /* both not packed */
	      buffer_temp_to = 
		PLA_malloc( (size_t) typesize * local_length * local_width );

	      PLA_MPI_Allreduce( buffer_temp_from, buffer_temp_to, 
			     local_length * local_width, datatype, op, comm );

	      PLA_Obj_set_local_contents( PLA_NO_TRANSPOSE, local_length,
					  local_width, buffer_temp_to, 
					  local_length, 1, Obj_to );
	     
	      PLA_free( buffer_temp_to );
	    }
	      
	    PLA_free( buffer_temp_from );
	  }
	  done = TRUE;
	}
	else {                                        /* Objects not aligned */
	  /*  NOT YET IMPLEMENTED.  USE OLD REDUCE */
	}
      } 
      else {                 /* Obj_from proj_onto_col, Obj_to proj_onto_row */
	int width;
	PLA_Obj Obj_mv = NULL;

	PLA_Obj_global_width( Obj_from, &width );
	PLA_Mvector_create_conf_to( Obj_from, width, &Obj_mv );
	
	PLA_Reduce_xx_from_dpmv_to_mv( shape, Obj_from, op, alpha,  Obj_mv );

	PLA_Copy( Obj_mv, Obj_to );

	PLA_Obj_free( &Obj_mv );
      }
      break;
    }                                       /* End of Obj_from PROJ_ONTO_COL */
  case PLA_PROJ_ONTO_ROW:                          /* Obj_from PROJ_ONTO_ROW */
    {
      if ( proj_onto_to == proj_onto_from ) {  /* Both dpmv's proj. onto row */
	int align_col_from, align_col_to;

	PLA_Obj_global_align_col( Obj_from, &align_col_from );
	PLA_Obj_global_align_col( Obj_to,   &align_col_to );
	if ( align_col_from == align_col_to ){ 	        /* Alignment matches */
	  PLA_Obj_datatype(     Obj_from, &datatype );
	  MPI_Type_size( datatype, &typesize );
	  PLA_Obj_local_length( Obj_from, &local_length );
	  PLA_Obj_local_width(  Obj_from, &local_width );
	  PLA_Obj_local_ldim(   Obj_from, &local_ldim_from );
	  PLA_Obj_local_ldim(   Obj_to,   &local_ldim_to );
	  PLA_Obj_local_buffer( Obj_from, &buffer_from );
	  PLA_Obj_local_buffer( Obj_to,   &buffer_to );
	  PLA_Obj_template(     Obj_from,   &templ );
	  PLA_Temp_comm_col ( templ, &comm );

	  if ( local_ldim_from == local_length ){
	    if ( local_ldim_to == local_length ){     /* both objects packed */
	      PLA_MPI_Allreduce( buffer_from, buffer_to, 
			     local_length * local_width, datatype, op, comm );
	    }
	    else  {                                     /* Obj_to not packed */
	      buffer_temp_to = 
		PLA_malloc( (size_t) typesize * local_length * local_width );

	      PLA_MPI_Allreduce( buffer_from, buffer_temp_to, 
			     local_length * local_width, datatype, op, comm );

	      pla_array_copy_new( datatype, local_length, local_width,
				  buffer_temp_to, local_length, buffer_to,
                                  local_ldim_to );

	      PLA_free( buffer_temp_to );
	    }
	  }                                           /* Obj_from not packed */
	  else  {
	    buffer_temp_from = 
	      PLA_malloc( (size_t) typesize * local_length * local_width );

	    PLA_Obj_get_local_contents( Obj_from, PLA_NO_TRANSPOSE, 
				        &local_length, &local_width, 
				        buffer_temp_from, 
				        local_length, 1 );

	    if ( local_ldim_to == local_length ){           /* Obj_to packed */
	      PLA_MPI_Allreduce( buffer_temp_from, buffer_to, 
			    local_length * local_width, 
			    datatype, op, comm );
	    }
	    else  {                                       /* both not packed */
	      buffer_temp_to = 
		PLA_malloc( (size_t) typesize * local_length * local_width );

	      PLA_MPI_Allreduce( buffer_temp_from, buffer_temp_to, 
			     local_length * local_width, datatype, op, comm );

	      PLA_Obj_set_local_contents( PLA_NO_TRANSPOSE, local_length,
					  local_width, buffer_temp_to, 
					  local_length, 1, Obj_to );
	     
	      PLA_free( buffer_temp_to );
	    }
	      
	    PLA_free( buffer_temp_from );
	  }
	  done = TRUE;
	}
	else {                                        /* Objects not aligned */
	  /*  NOT YET IMPLEMENTED.  USE OLD REDUCE */
	}
      } 
      else {                 /* Obj_from proj_onto_row, Obj_to proj_onto_col */
	int width;
	PLA_Obj Obj_mv = NULL;

	PLA_Obj_global_width( Obj_to, &width );
	PLA_Mvector_create_conf_to( Obj_to, width, &Obj_mv );
	
	PLA_Reduce_xx_from_dpmv_to_mv( shape, Obj_from, op, alpha,  Obj_mv );

	PLA_Copy( Obj_mv, Obj_to );

	PLA_Obj_free( &Obj_mv );
      }
      break;
    }                                       /* End of Obj_from PROJ_ONTO_COL */
  }

  if ( report_case ) {
    printf("PLA_Reduce_xx_from_dpmv_to_dpmv not yet implemented\n");
    report_case = FALSE;
  }
  if ( shape, !done ) return PLA_Reduce_old( Obj_from, op, alpha,  Obj_to );
  else return( PLA_SUCCESS );
}

