#include <mps/mps.h>
#include <check.h>
#include "check_implementation.h"

START_TEST (basics_allocate_destroy)
{
  mps_context * ctx = mps_context_new (); 
  mps_monomial_matrix_poly *mp = mps_monomial_matrix_poly_new (ctx, 10, 120, false); 
  mps_monomial_matrix_poly_free (ctx, MPS_POLYNOMIAL (mp)); 
  mps_context_free (ctx); 
}
END_TEST

START_TEST (determinant_mhessenberg_example1)
{
  /* Custom 8 by 8 example. The matrix is defined as: 
   * A(i,j) = sin(i) * cos(j) + 1e-3 * i*j . 
   * Its determinant should be*/ 
  mpc_t *hessenberg_matrix = mps_newv (mpc_t, 64); 
  mpc_t det, t; 
  rdpe_t diff, mod; 
  int i, j; 

  mpc_vinit2 (hessenberg_matrix, 64, DBL_MANT_DIG); 
  mpc_init2 (det, DBL_MANT_DIG); 
  mpc_init2 (t, DBL_MANT_DIG); 

  mps_context *ctx = mps_context_new (); 

  for (i = 0; i < 8; i++)
    for (j = MAX(0, i-1); j < 8; j++)
      {
	mpc_set_d (hessenberg_matrix[i*8 + j], sin(1.0 * (i+1)) * cos(1.0 * (j+1)) + 1e-3 * (i+1) * (j+1), 0.0); 
      }

  mps_mhessenberg_determinant (ctx, hessenberg_matrix, 8, det); 
  
  mpc_vclear (hessenberg_matrix, 64); 
  free (hessenberg_matrix); 

  mpc_set_d  (t, 6.14427105181099e-06, 0.0);
  mpc_sub_eq (det, t); 

  mpc_rmod (diff, det); 
  mpc_rmod (mod, t); 
	  
  fail_unless (rdpe_get_d (diff) < rdpe_get_d (mod) * 10.0 * 8 * DBL_EPSILON, 
	       "The error on determinant_hessenberg_example1 is bigger than n * DBL_EPSILON"); 

  mps_context_free (ctx); 
}
END_TEST

START_TEST (determinant_shifted_hessenberg_example1)
{
  /* Custom 8 by 8 example. The matrix is defined as: 
   * A(i,j) = sin(i) * cos(j) + 1e-3 * i*j . 
   * Its determinant should be*/ 
  cplx_t *hessenberg_matrix = mps_newv (cplx_t, 64); 
  cplx_t det, t; 
  int i, j; 

  cplx_t shifts[3]; 
  cplx_t results[3]; 

  cplx_set_d (shifts[0], 0.403815598068559, 0.754480932782281); 
  cplx_set_d (results[0], -0.2755152414594506,  0.0732925950505913); 

  cplx_set_d (shifts[1], 0.0590780603923638, 0.9236523504901163); 
  cplx_set_d (results[1], 0.5885575152394473, -0.0800261442305445); 

  cplx_set_d (shifts[2], 0.0534877455734864, 0.1853972552409148); 
  cplx_set_d (results[2], -4.28682106680713e-05, -4.18995301563591e-05); 

  mps_context *ctx = mps_context_new (); 

  for (i = 0; i < 8; i++)
    for (j = MAX(0, i-1); j < 8; j++)
      {
	cplx_set_d (hessenberg_matrix[i*8 + j], sin(1.0 * (i+1)) * cos(1.0 * (j+1)) + 1e-3 * (i+1) * (j+1), 0.0); 
      }

  for (i = 0; i < 2; i++)
    {
      mps_fhessenberg_shifted_determinant (ctx, hessenberg_matrix, shifts[i], 8, det); 
      cplx_sub_eq (det, results[i]); 

      fail_unless (cplx_mod (det) < cplx_mod (results[i]) * 10.0 * 8 * DBL_EPSILON, 
		   "The error on shifted Hessenberg determinant example1 is bigger than n * DBL_EPSILON"); 
    }
  
  free (hessenberg_matrix); 
  mps_context_free (ctx); 
}
END_TEST

START_TEST (determinant_hessenberg_example1)
{
  /* Custom 8 by 8 example. The matrix is defined as: 
   * A(i,j) = sin(i) * cos(j) + 1e-3 * i*j . 
   * Its determinant should be*/ 
  cplx_t *hessenberg_matrix = mps_newv (cplx_t, 64); 
  cplx_t det, t; 
  int i, j; 

  mps_context *ctx = mps_context_new (); 

  for (i = 0; i < 8; i++)
    for (j = MAX(0, i-1); j < 8; j++)
      {
	cplx_set_d (hessenberg_matrix[i*8 + j], sin(1.0 * (i+1)) * cos(1.0 * (j+1)) + 1e-3 * (i+1) * (j+1), 0.0); 
      }

  mps_fhessenberg_determinant (ctx, hessenberg_matrix, 8, det); 
  
  free (hessenberg_matrix); 

  cplx_set_d  (t, 6.14427105181099e-06, 0.0);
  cplx_sub_eq (det, t); 
	  
  fail_unless (cplx_mod (det) < cplx_mod (t) * 10.0 * 8 * DBL_EPSILON, 
	       "The error on determinant_hessenberg_example1 is bigger than n * DBL_EPSILON"); 

  mps_context_free (ctx); 
}
END_TEST

START_TEST (determinant_shifted_mhessenberg_example1)
{
  /* Custom 8 by 8 example. The matrix is defined as: 
   * A(i,j) = sin(i) * cos(j) + 1e-3 * i*j . 
   * Its determinant should be*/ 
  mpc_t *hessenberg_matrix = mps_newv (mpc_t, 64); 
  mpc_t det, t; 
  int i, j; 

  mpc_t shifts[3]; 
  mpc_t results[3]; 

  mpc_vinit2 (hessenberg_matrix, 64, DBL_MANT_DIG); 
  mpc_init2 (det, DBL_MANT_DIG); 
  mpc_init2 (t, DBL_MANT_DIG); 
  mpc_vinit2 (shifts, 3, DBL_MANT_DIG); 
  mpc_vinit2 (results, 3, DBL_MANT_DIG); 

  mpc_set_d (shifts[0], 0.403815598068559, 0.754480932782281); 
  mpc_set_d (results[0], -0.2755152414594506,  0.0732925950505913);

  mpc_set_d (shifts[1], 0.0590780603923638, 0.9236523504901163); 
  mpc_set_d (results[1], 0.5885575152394473, -0.0800261442305445); 

  mpc_set_d (shifts[2], 0.0534877455734864, 0.1853972552409148); 
  mpc_set_d (results[2], -4.28682106680713e-05, -4.18995301563591e-05); 

  mps_context *ctx = mps_context_new (); 

  for (i = 0; i < 8; i++)
    for (j = MAX(0, i-1); j < 8; j++)
      {
	mpc_set_d (hessenberg_matrix[i*8 + j], sin(1.0 * (i+1)) * cos(1.0 * (j+1)) + 1e-3 * (i+1) * (j+1), 0.0); 
      }

  for (i = 0; i < 2; i++)
    {
      rdpe_t diff, mod; 

      mps_mhessenberg_shifted_determinant (ctx, hessenberg_matrix, shifts[i], 8, det); 
      mpc_sub_eq (det, results[i]); 

      mpc_rmod (diff, det); 
      mpc_rmod (mod, results[i]); 

      printf ("%d: ", i); mpc_out_str_2 (stdout, 10, 15, 15, det); printf ("\n"); 

      fail_unless (rdpe_get_d (diff) < rdpe_get_d (mod) * 10.0 * 8 * DBL_EPSILON, 
		   "The error on shifted Hessenberg determinant example1 is bigger than n * DBL_EPSILON"); 
    }

  mpc_vclear (shifts, 3); 
  mpc_vclear (results, 3); 
  mpc_vclear (hessenberg_matrix, 64); 
  mpc_clear (t);
  mpc_clear (det);
  
  free (hessenberg_matrix);
  mps_context_free (ctx);
}
END_TEST

int 
main (void) 
{
  int number_failed;

  starting_setup ();

  Suite *s = suite_create ("Matrices");
  TCase *tc_basics = tcase_create ("Basic operations"); 
  TCase *tc_determinant = tcase_create ("Determinant computation");

  // Basic operation
  tcase_add_test (tc_basics, basics_allocate_destroy); 

  // Add tests of the deteminant
  tcase_add_test (tc_determinant, determinant_hessenberg_example1); 
  tcase_add_test (tc_determinant, determinant_shifted_hessenberg_example1); 
  tcase_add_test (tc_determinant, determinant_mhessenberg_example1); 
  tcase_add_test (tc_determinant, determinant_shifted_mhessenberg_example1); 

  suite_add_tcase (s, tc_determinant);

  SRunner *sr = srunner_create (s);
  srunner_run_all (sr, CK_NORMAL);
  number_failed = srunner_ntests_failed (sr);
  srunner_free (sr);

  return (number_failed != 0);
}