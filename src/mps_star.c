/***********************************************************
**       Multiprecision Polynomial Solver (MPSolve)       **
**                 Version 2.2, May 2001                  **
**                                                        **
**                      Written by                        **
**       Dario Andrea Bini and Giuseppe Fiorentino        **
**       (bini@dm.unipi.it)  (fiorent@dm.unipi.it)        **
**                                                        **
** (C) 2001, Dipartimento di Matematica, FRISCO LTR 21024 **
***********************************************************/

/**
 * @file 
 * @brief Routines to compute starting approximations
 * for the algorithm
 *
 */

#include <mps/mps.h>

static const double pi2 = 6.283184;

/* forward declaration */
void mps_raisetemp(mps_status* s, unsigned long int digits);
void mps_raisetemp_raw(mps_status* s, unsigned long int digits);

/**
 * @brief Compute the greatest common divisor of <code>a</code>
 * and <code>b</code>.
 *
 * @param a first integer
 * @param b second integer
 * @return <code>GCD(a,b)</code>
 */
int mps_gcd(int a, int b) {
  int temp;
  do {
	temp = b;
	b = a % b;
	a = temp;
  } while (b != 0);
  return a;
}

/**
 * @brief Find the sigma that maximize distance between
 * starting approximation in the last annulus and the one
 * in the new annulus.
 *
 * This function also set <code>s->last_sigma</code> to the value
 * that is computed.
 *
 * @param s the mps_status struct pointer.
 * @param last_sigma the last value of sigma.
 * @param i_cluster the index of the cluster we are analyzing.
 * @param n the number of roots in the cluster.
 * @return the shift advised for the starting approximation in the annulus.
 */
double mps_maximize_distance(mps_status* s, double last_sigma,
		                     int i_cluster, int n) {
	double delta_sigma;

	/* Find number of roots in the last cluster */
	int old_clust_n = s->punt[i_cluster] - s->punt[i_cluster - 1];

	/* Compute right shifting angle for the new approximations, i.e.
	 * pi / [m,n] where [m,n] is the least common multiply of m and n.
	 * This is done by computing the gcd and then dividing m*n by it. */
	delta_sigma = PI * (old_clust_n * mps_gcd(old_clust_n, n)) / (4 * n);

	/* Return shifted value, archiving it for the next pass */
	s->last_sigma = last_sigma + delta_sigma;
	return s->last_sigma;
}

/**
 * @brief Compute radii of the circles where the initial approximation
 * will be disposed by mps_fstart()
 *
 * @param s mps_status* stuct pointer.
 * @param n number of roots in the cluster.
 * @param i_clust Index of the cluster to analyze.
 * @param clust_rad radius of the cluster.
 * @param g new gravity center where the polynomial has been shifted.
 * @param eps out epsilon.
 * @param fap[] Array with the moduli of the coefficients.
 *
 * @see mps_fstart()
 */
void mps_fcompute_starting_radii(mps_status* s, int n, int i_clust, double clust_rad,
		double g, rdpe_t eps, double fap[]) {

	  const double  big = DBL_MAX,   small = DBL_MIN;
	  const double xbig = log(big), xsmall = log(small);

	  int i, j, k, nzeros, iold, ni, offset;
	  double temp, r;

	  ni = 0;
	  nzeros = 0;
	  r = 0.0;

	  /**********************************************
	    check for possible null entries in the trailing
	    coefficients only in the case where the polynomial
	    has been shifted in g, replace null coefficients
	    with small numbers according to the working precision
	    and to the number of null coefficients
	    **********************************************/
	  if (g != 0.0) {
	    for (i = 0; i <= n; i++)
	      if (fap[i] != 0.0) {
		ni = i;
		break;
	      }
	    if (ni == 0)
	      temp = 2 * xsmall;
	    else
	      temp = log(fap[ni]) + ni * (log(DBL_EPSILON) + log(g * ni * 10.0));
	  } else
	    temp = 2 * xsmall;

	  for (i = 0; i <= n; i++)
	    if (fap[i] != 0.0)
	      s->fap2[i] = log(fap[i]);
	    else
	      s->fap2[i] = temp;

	  /* Compute convex hull */
	  mps_fconvex(s, n, s->fap2);

	  /* compute the radii of the circles containing starting approximations  */
	  s->n_radii = 0;
	  s->partitioning[0] = 0;
	  for (i = 1; i <= n; i++)
	    if (s->h[i]) {
	    	iold = s->partitioning[s->n_radii];
	    	nzeros = i - iold;
	    	temp = (s->fap2[iold] - s->fap2[i]) / nzeros;
	    	/* if the radius is too small to be represented as double, set it
	    	 * to the minimum  representable double */
	    	if (temp < xsmall)	/* if (temp < MAX(xsmall, -xbig)) DARIO Giugno 23 */
	    		r = DBL_MIN;		/* r = small; */

	    	/* if the radius is too big to be represented as double, set it
	    	 * to the maximum representable double */
	    	if (temp > xbig)
	    		r = DBL_MAX;		/* big;   DARIO Giugno 23 */

	    	/* if the radius is representable as double, compute it    */
	    	if ((temp <= xbig) && (temp > xsmall))
	    		/* if ((temp <= xbig) && (temp > MAX(-xbig, xsmall))) DARIO Giugno 23 */
	    		r = exp(temp);

	    	/* if the radius is greater than the radius of the cluster
			 * set the radius equal to the radius of the cluster */
			if (clust_rad != 0 && r > clust_rad)
				  r = clust_rad;

			s->fradii[s->n_radii] = r;
			s->partitioning[++s->n_radii] = i;
	    }

	  /* Close partitioning */
	  s->partitioning[s->n_radii] = n;

	  /* Compact radius that are too near */
	  for(i = 0; i < s->n_radii; i++) {
		  /* Scan next radii to see if they are near the
		   * i-th that we are considering now  */
		  for(j = i+1; j < s->n_radii; j++) {
			  if ((s->fradii[j] - s->fradii[i]) / s->fradii[i] > s->circle_relative_distance) {
				  break;
			  }
		  }

		  /* This is the number of circles that are near */
		  offset = j - i;

		  if(s->DOLOG && (offset > 1)) {
			  fprintf(s->logstr,
					  "    MPS_FCOMPUTE_STARTING_RADII: Compacting circles"
					  " from %d to %d\n", i, j);
		  }

		  /* We shall now compact circles between i and j, so
		   * we start computing the mean of the radius */
		  for(k = i+1; k < j; k++) {
			  s->fradii[i] += s->fradii[k];
		  }
		  s->fradii[i] /= offset;
		  s->partitioning[i+1] = s->partitioning[j];

		  /* Move other circles backward */
		  for(k = j; k < s->n_radii; k++) {
			  s->fradii[k - offset + 1] = s->fradii[k];
			  s->partitioning[k - offset + 1] = s->partitioning[k];
		  }

		  /* Set new s->n_radii and new partitioning */
		  s->n_radii = s->n_radii - offset + 1;
	  }
}

/**
 * @brief Compute new starting approximations to the roots
 * of the polynomial \f$p(x)\f$ having coefficients of modulus apoly.
 *
 * Computations is done by
 * means of the Rouche'-based criterion of Bini (Numer. Algo. 1996). 
 * The program can compute all the approximations
 * (if \f$n\f$ is the degree of \f$p(x)\f$) or it may compute the
 * approximations of the cluster of index <code>i_clust</code>
 * The status vector is changed into <code>'o'</code> for the components
 * that belong to a cluster with relative radius less than <code>eps</code>.
 * The status vector is changed into <code>'x'</code> for the components that
 * cannot be represented as double.
 *
 * @param n number of roots in the cluster.
 * @param i_clust index of cluster to analyze.
 * @param clust_rad radius of cluster.
 * @param g gravity center of the cluster.
 * @param eps a double that represent the maximum value
 * of relative radius (with respect to <code>g</code>) of
 * roots whose status must be set to <code>o</code>.
 * @param fap array of moduli of the coefficients as double.
 *
 * @see status
 */
void
mps_fstart(mps_status* s, int n, int i_clust, double clust_rad,
		   double g, rdpe_t eps, double fap[])
{


  int i,j, jj, l, nzeros = 0;
  double sigma, th, ang, r = 0;
  rdpe_t tmp;

  if (s->random_seed)
    sigma = drand();
  else {
	/* If this is the first cluster select sigma = 0. In the other
	 * case try to maximize starting points distance. */
    if (i_clust == 0) {
    	sigma = s->last_sigma = 0;
    } else {
    	sigma = mps_maximize_distance(s, s->last_sigma, i_clust, n);
    }
  }

  th = pi2 / n;


  /* In the case of user-defined polynomial choose as starting
   * approximations equally spaced points in the unit circle.  */
  if (s->data_type[0] == 'u') {
    ang = pi2 / n; 
    for (i = 0; i < n; i++)
      cplx_set_d(s->froot[i], cos(ang * i + sigma), sin(ang * i + sigma));
    return;
  }

  /* In the general case apply the Rouche-based criterion */

  /* Compute starting radii */
  mps_fcompute_starting_radii(s, n, i_clust, clust_rad, g, eps, fap);

  for(i = 0; i < s->n_radii; i++) {
	  nzeros = s->partitioning[i+1] - s->partitioning[i];
	  ang = pi2 / nzeros;
	  r = s->fradii[i];

	  for (j = s->partitioning[i]; j < s->partitioning[i+1]; j++) {
		  if (g != 0.0)
			  l = s->clust[s->punt[i_clust] + j];
		  else
			  l = j;
		  jj = j - s->partitioning[i];

	  /* if the radius reaches extreme values then set the component
	   * of status, corresponding to approximation which fall out the
	   * representable range, to 'x' (out)    */
	  if ((r == DBL_MIN) || (r == DBL_MAX))
		  /* if ((r == small) || (r == big)) DARIO Giugno 23 */
		  s->status[l][0] = 'x';
	  cplx_set_d(s->froot[l], r * cos(ang * jj + th * s->partitioning[i+1] + sigma),
			  r * sin(ang * jj + th * s->partitioning[i+1] + sigma));
  }


	  /* If the new radius of the cluster is relatively smaller, then
	   * set the status component equal to 'o' (output) */
	  if (g != 0.0) {
		rdpe_mul_d(tmp, eps, g);
		if (r * nzeros <= rdpe_get_d(tmp))
		  for (j = 0; j < s->punt[i_clust + 1] - s->punt[i_clust]; j++) {
		l = s->clust[s->punt[i_clust] + j];
		s->status[l][0] = 'o';
		s->frad[l] = r * nzeros;
		  }
	  }

  }
}

/**
 * @brief Compute radii of the circles where the initial approximation
 * will be disposed by mps_dstart()
 *
 * @param s mps_status* stuct pointer.
 * @param n number of roots in the cluster.
 * @param i_clust Index of the cluster to analyze.
 * @param clust_rad radius of the cluster.
 * @param g new gravity center where the polynomial has been shifted.
 * @param eps out epsilon.
 * @param dap[] Array with the moduli of the coefficients.
 *
 * @see mps_dstart()
 */
void mps_dcompute_starting_radii(mps_status* s, int n, int i_clust, rdpe_t clust_rad,
		rdpe_t g, rdpe_t eps, rdpe_t dap[]) {

	/* Compute big and small values */
	double  xbig, xsmall;
	xbig = rdpe_log(RDPE_MAX);
	xsmall = rdpe_log(RDPE_MIN);


	int i, j, k, nzeros, iold, ni, offset;
	double temp;
	rdpe_t r, tmp;

	ni = 0;
	nzeros = 0;
	rdpe_set(r, rdpe_zero);

	if (rdpe_ne(g, rdpe_zero)) {
		for (i = 0; i <= n; i++)
			if (rdpe_ne(dap[i], rdpe_zero)) {
				ni = i;
				break;
			}
		if (ni == 0)
			temp = -2.0 * (LONG_MAX * LOG2);
		else {
			/* temp = log(dap[ni])+ni*(log(DBL_EPSILON)+log(g*ni*10.d0)) */
			temp = ni * 10.0;
			rdpe_mul_d(tmp, g, temp);
			temp = rdpe_log(tmp);
			temp += log(DBL_EPSILON);
			temp *= ni;
			temp += rdpe_log(dap[ni]);
		}
	} else
		temp = -2.0 * (LONG_MAX * LOG2);
	for (i = 0; i <= n; i++)
		if (rdpe_ne(dap[i], rdpe_zero))
			s->fap2[i] = rdpe_log(dap[i]);
		else
			s->fap2[i] = temp;

	/* compute the convex hull */
	mps_fconvex(s, n, s->fap2);

	/* compute the radii of the circles containing starting approximations  */
	s->n_radii = 0;
	s->partitioning[0] = 0;
	for (i = 1; i <= n; i++)
	    if (s->h[i]) {
	    	iold = s->partitioning[s->n_radii];
	    	nzeros = i - iold;
	    	temp = (s->fap2[iold] - s->fap2[i]) / nzeros;
	    	/* if the radius is too small to be represented as double, set it
	    	 * to the minimum  representable double */
	    	if (temp < xsmall)
	    		rdpe_set(r, RDPE_MIN);		/* r = small; */

	    	/* if the radius is too big to be represented as double, set it
	    	 * to the maximum representable double */
	    	if (temp < xbig)
	    		rdpe_set(r, RDPE_MAX);

	    	/* if the radius is representable as double, compute it    */
	    	if ((temp < xbig) && (temp > xsmall))
	    		rdpe_set_d(r, temp);
	    		rdpe_exp_eq(r);


	    	/* if the radius is greater than the radius of the cluster
			 * set the radius equal to the radius of the cluster */
			  if (rdpe_ne(clust_rad, rdpe_zero) && rdpe_gt(r, clust_rad))
				  rdpe_set(r, clust_rad);

			  rdpe_set(s->dradii[s->n_radii], r);
			  s->partitioning[++s->n_radii] = i;
	    }

	/* Close partitioning */
	s->partitioning[s->n_radii] = n;

	/* Compact radius that are too near */
	for(i = 0; i < s->n_radii; i++) {
		/* Scan next radii to see if they are near the
		 * i-th that we are considering now  */
		for(j = i+1; j < s->n_radii; j++) {
			rdpe_sub(tmp, s->dradii[j],s->dradii[i]);
			rdpe_div_eq(tmp, s->dradii[i]);
			if (rdpe_get_d(tmp) > s->circle_relative_distance) {
				break;
			}
		}

		/* This is the number of circles that are near */
		offset = j - i;

		/* We shall now compact circles between i and j, so
		 * we start computing the mean of the radius */
		for(k = i+1; k < j; k++) {
			rdpe_add_eq(s->dradii[i], s->dradii[j]);
		}

		rdpe_div_eq_d(s->dradii[i], offset);
		s->partitioning[i+1] = s->partitioning[j];

		/* Move other circles backward */
		for(k = j; k < s->n_radii; k++) {
			rdpe_set(s->dradii[k - offset + 1], s->dradii[k]);
			s->partitioning[k - offset + 1] = s->partitioning[k];
		}

		/* Set new s->n_radii and new partitioning */
		s->n_radii = s->n_radii - offset + 1;
  }
}

/**
 * @brief Compute new starting approximations to the roots of the
 * polynomial \f$p(x)\f$ having coefficients of modulus apoly, by
 * means of the Rouche'-based criterion of Bini (Numer. Algo. 1996).
 *
 * The program can compute all the approximations
 * (if \f$n\f$ is the degree of \f$p(x)\f$) or it may compute the
 * approximations of the cluster of index \f$i_clust\f$
 * The status vector is changed into <code>'o'</code> for the components
 * that belong to a cluster with relative radius less than <code>eps</code>.
 * The status vector is changed into <code>'f'</code> for the components
 * that cannot be represented as <code>>dpe</code>.
 *
 * @param s mps_status struct pointer.
 * @param n number of root in the cluster to consider
 * @param i_clust index of the cluster to consider.
 * @param clust_rad radius of the cluster.
 * @param g new center in which the the polynomial will be shifted.
 * @param eps maximum radius considered small enough to be negligible.
 * @param dap[] moduli of the coefficients as <code>dpe</code> numbers.
 */
void
mps_dstart(mps_status* s, int n, int i_clust, rdpe_t clust_rad,
		   rdpe_t g, rdpe_t eps, rdpe_t dap[])
{
  int l, i, j, jj, iold, nzeros = 0;
  rdpe_t r, tmp, tmp1;
  double sigma, th, ang;
  boolean flag = false;

  if (s->random_seed)
    sigma = drand();
  else {
	/* If this is the first cluster select sigma = 0. In the other
	 * case try to maximize starting points distance. */
    if (i_clust == 0) {
    	sigma = s->last_sigma = 0;
    } else {
    	sigma = mps_maximize_distance(s, s->last_sigma, i_clust, n);
    }
  }

  /* In the case of user-defined polynomial choose as starting
   * approximations equispaced points in the unit circle. */
  if (s->data_type[0] == 'u') {
    ang = pi2 / n;
    for (i = 0; i < n; i++)
      cdpe_set_d(s->droot[i], cos(ang * i + sigma), 
		 sin(ang * i + sigma));
    return;
  }

  /* check if it is the case dpe_after_float, in this case set flag=true  */
  for (i = 0; i < n; i++) {
    flag = (s->status[i][0] == 'x');
    if (flag)
      break;
  }

  /* Compute starting radii with the Rouche based criterion */
  mps_dcompute_starting_radii(s, n, i_clust, clust_rad, g, eps, dap);
  th = pi2 / n;

  /* Scan all the vertices of the convex hull   */
  iold = 0;
  for (i = 1; i <= n; i++)
    if (s->h[i]) {

    	iold = s->partitioning[i];
    	nzeros = s->partitioning[i+1] - iold;
    	rdpe_set(r, s->dradii[i]);

    	ang = pi2 / nzeros;
    	for (j = iold; j < i; j++) {
    		if (rdpe_ne(g, rdpe_zero))
    			l = s->clust[s->punt[i_clust] + j];
    		else
    			l = j;

    		jj = j - iold;

    		/* If dpe_after_float (i.e., flag is true) recompute the starting
    		 * values of only the approximations falling out of the range */
    		if (flag) {
    			if (s->status[l][0] == 'x') {
    				cdpe_set_d(s->droot[l], cos(ang * jj + th * s->partitioning[i+1] + sigma),
    						sin(ang * jj + th * s->partitioning[i+1] + sigma));
    				cdpe_mul_eq_e(s->droot[l], r);
    				s->status[l][0] = 'c';
    				/*#G 27/4/98 if (rdpe_eq(r, big) || rdpe_eq(r, small)) */
    				if (rdpe_eq(r, RDPE_MIN) || rdpe_eq(r, RDPE_MAX))
    					s->status[l][0] = 'f';
    			}
    		} else {
    			/* else compute all the initial approximations */
    			cdpe_set_d(s->droot[l], cos(ang * jj + th * i + sigma),
    					sin(ang * jj + th * i + sigma));
    			cdpe_mul_eq_e(s->droot[l], r);
    			/*#G 27/4/98 if (rdpe_eq(r, big) || rdpe_eq(r, small)) */
    			if (rdpe_eq(r, RDPE_MIN) || rdpe_eq(r, RDPE_MAX))
    				s->status[l][0] = 'f';
    		}
    	}
    	iold = i;

	  /* If the new radius of the cluster is relatively small, then
	   * set the status component equal to 'o' (output) */
	  if (rdpe_ne(g, rdpe_zero)) {
		  rdpe_mul(tmp, g, eps);
		  rdpe_mul_d(tmp1, r, (double) nzeros);
		  if (rdpe_lt(tmp1, tmp))
			  for (j = 0; j <= s->punt[i_clust + 1] - s->punt[i_clust]; j++) {
				  l = s->clust[s->punt[i_clust] + j];
				  s->status[l][0] = 'o';
				  rdpe_set(s->drad[l], tmp1);
			  }
	  }

}
}

/**
 * @brief Compute radii of the circles where the initial approximation
 * will be disposed by mps_mstart()
 *
 * @param s mps_status* stuct pointer.
 * @param n number of roots in the cluster.
 * @param i_clust Index of the cluster to analyze.
 * @param clust_rad radius of the cluster.
 * @param g new gravity center where the polynomial has been shifted.
 * @param eps out epsilon.
 * @param dap[] Array with the moduli of the coefficients.
 *
 * @see mps_mstart()
 */
void
mps_mcompute_starting_radii(mps_status* s, int n, int i_clust, rdpe_t clust_rad,
		rdpe_t g, rdpe_t dap[]) {

	int i, offset, iold, nzeros, j, k;
	rdpe_t big, small, tmp;
	double xbig, xsmall, temp;

	xsmall = rdpe_log(RDPE_MIN);
	xbig = rdpe_log(RDPE_MAX);
	rdpe_set(small, RDPE_MIN);
	rdpe_set(big, RDPE_MAX);

	if (rdpe_eq(dap[0], rdpe_zero))
	    s->fap2[0] = -s->mpwp * LOG2;

	  /*  check for possible null entries in the trailing coefficients */
	  for (i = 0; i <= n; i++)
	    if (rdpe_ne(dap[i], rdpe_zero))
	      s->fap2[i] = rdpe_log(dap[i]);
	    else
	      s->fap2[i] = s->fap2[0];

	  /* compute the convex hull */
	  mps_fconvex(s, n, s->fap2);

	  /* Scan all the vertices of the convex hull */
	  s->partitioning[0] = 0;
	  s->n_radii = 0;
	  for (i = 1; i <= n; i++) {
			if (s->h[i]) {
				iold = s->partitioning[s->n_radii];
				nzeros = i - iold;
				temp = (s->fap2[iold] - s->fap2[i]) / nzeros;
				/* if the radius is too small or too big to be represented as dpe,
				 * output a warning message */
				if (temp < xsmall) {
					rdpe_set(s->dradii[s->n_radii], small);
					if (s->DOLOG) {
						fprintf(s->logstr, "Warning: Some zeros are too small to be\n");
						fprintf(s->logstr, " represented as cdpe, they are replaced by\n");
						fprintf(s->logstr, " small numbers and the status is set to 'F'.\n");
					}
				}
				if (temp > xbig) {
					rdpe_set(s->dradii[s->n_radii], big);
					if (s->DOLOG) {
						fprintf(s->logstr, "Warning: Some zeros are too big to be\n");
						fprintf(s->logstr, " represented as cdpe, they are replaced by\n");
						fprintf(s->logstr, " big numbers and the status is set to 'F'.\n");
					}
				}

				/* if the radius is representable as dpe, compute it */
				if (temp <= xbig && temp >= xsmall) {
					rdpe_set_d(s->dradii[s->n_radii], temp);
					rdpe_exp_eq(s->dradii[s->n_radii]);
				}
				/* if the radius is greater than the radius of the cluster
				 * set the radius equal to the radius of the cluster */
				if (rdpe_gt(s->dradii[s->n_radii], clust_rad))
					rdpe_set(s->dradii[s->n_radii], clust_rad);

				/* Close partitioning and start a new one */
				s->partitioning[++s->n_radii] = i;
			}
	  }

	  /* Set last point of the partitioning */
	  s->partitioning[s->n_radii] = n;

	  /* Compact radius that are too near */
	  for(i = 0; i < s->n_radii; i++) {
		  /* Scan next radii to see if they are near the
		   * i-th that we are considering now  */
		  for(j = i+1; j < s->n_radii; j++) {
			  rdpe_sub(tmp, s->dradii[j],s->dradii[i]);
			  rdpe_div_eq(tmp, s->dradii[i]);
			  if (rdpe_get_d(tmp) > s->circle_relative_distance) {
				  break;
			  }
		  }

		  /* This is the number of circles that are near */
		  offset = j - i;

		  if(s->DOLOG && offset > 1) {
			  fprintf(s->logstr,
					  "MPS_MCOMPUTE_STARTING_RADII: Compacting disc from %d to %d\n",
					  i, j);
		  }

		  /* We shall now compact circles between i and j, so
		   * we start computing the mean of the radius */
		  for(k = i+1; k < j; k++) {
			  rdpe_add_eq(s->dradii[i], s->dradii[j]);
		  }

		  rdpe_div_eq_d(s->dradii[i], offset);
		  s->partitioning[i+1] = s->partitioning[j];

		  /* Move other circles backward */
		  for(k = j; k < s->n_radii; k++) {
			  rdpe_set(s->dradii[k - offset + 1], s->dradii[k]);
			  s->partitioning[k - offset + 1] = s->partitioning[k];
		  }

		  /* Set new s->n_radii and new partitioning */
		  s->n_radii = s->n_radii - offset + 1;
	  }
}

/**
 * @brief Multiprecision version of mps_fstart()
 * @see mps_fstart()
 */
void
mps_mstart(mps_status* s, int n, int i_clust, rdpe_t clust_rad,
		   rdpe_t g, rdpe_t dap[])
{

  int i, j, jj, iold, l, nzeros;
  double sigma, ang, th, temp;
  rdpe_t r, big, small, rtmp1, rtmp2;
  cdpe_t ctmp;

  rdpe_set(small, RDPE_MIN);
  rdpe_set(big, RDPE_MAX);



  if (s->random_seed)
    sigma = drand();
  else {
	/* If this is the first cluster select sigma = 0. In the other
	 * case try to maximize starting points distance. */
    if (i_clust == 0) {
    	sigma = s->last_sigma = 0;
    } else {
    	sigma = mps_maximize_distance(s, s->last_sigma, i_clust, n);
    }
  }

  nzeros = 0;
  temp = 0.0;

  /* In the general case apply the Rouche-based criterion */
  mps_mcompute_starting_radii(s, n, i_clust, clust_rad, g, dap);

  th = pi2 / n;

  /* Set initial approximations accordingly to the computed
   * circles  */
  for(i = 0; i < s->n_radii; i++) {
	  nzeros = s->partitioning[i+1] - s->partitioning[i];
	  ang = pi2 / nzeros;
	  iold = s->partitioning[i];

      /* Compute the initial approximations */
      for (j = iold; j < s->partitioning[i+1]; j++) {
    	  jj = j - iold;

    	  /* Take index relative to the cluster
    	   * that we are analyzing. */
    	  l = s->clust[s->punt[i_clust] + j];

    	  cdpe_set_d(ctmp, cos(ang * jj + th * s->partitioning[i+1] + sigma),
    			  sin(ang * jj + th * s->partitioning[i+1] + sigma));
    	  cdpe_mul_eq_e(ctmp, s->dradii[i]);
    	  cdpe_set(s->droot[l], ctmp);

    	  if (rdpe_eq(s->dradii[i], big) || rdpe_eq(s->dradii[i], small)) {
    		  s->status[l][0] = 'f';
    	  }
      }
      iold = i;


	  /* If the new radius of the cluster is relatively small, then
	   * set the status component equal to 'o' (output)
	   * and set the corresponding radius */
	  rdpe_set(rtmp1, s->dradii[i]);
	  rdpe_mul_eq_d(rtmp1, (double) nzeros);
	  rdpe_set(rtmp2, g);
	  rdpe_mul_eq(rtmp2, s->eps_out);
	  if (rdpe_le(rtmp1, rtmp2))
		for (j = 0; j < s->punt[i_clust + 1] - s->punt[i_clust]; j++) {
		  l = s->clust[s->punt[i_clust] + j];
		  s->status[l][0] = 'o';
		  rdpe_mul_d(s->drad[l], r, (double) nzeros);
		}
	  rdpe_set(clust_rad,s->dradii[i]);
  }
}

/***********************************************************
*                     Subroutine FRESTART                  *
************************************************************
 The program scans the existing clusters and  selects the
 ones where shift in the gravity center must be done.
 Then computes the gravity center g, performs the shift of
 the variable and compute new starting approximations in the
 cluster.
 The components of the vector status(:,1) are set to 'c'
 (i.e., Aberth's iteration must be applied) if the cluster
 intersects the origin (in this case shift is not applied),
 or if new starting approximations have been selected.
 The gravity center g is choosen as a zero of the (m-1)-st
 derivative of the polynomial in the cluster, where m=m_clust
 is the multiplicity of the cluster. 

 Shift in g is perfomed if
        status=(c*u) for goal=count
        status=(c*u) or status=(c*i) for goal=isolate/approximate
 To compute g, first compute the weighted mean (super center sc)
 of the approximations in the cluster, where the weight are the
 radii, then compute the radius (super radius sr) of the disk
 centered in the super center containing all the disks of the cluster.
 Apply few steps of Newton's iteration to the (m-1)-st derivative
 of the polynomial starting from the super center and obtain the
 point g where to shift the variable.
 If g is outside the super disk of center sc and radius sr
 output a warning message.
 ***********************************************************/
void
mps_frestart(mps_status* s)
{
  int i, k, j, l, jj;
  double sr, sum, rad, rtmp, rtmp1;
  cplx_t sc, g, corr, ctmp;
  boolean tst, cont;

  /* For user's polynomials skip the restart stage (not yet implemented) */
  if (s->data_type[0] == 'u')
    return;

  /* scan the existing clusters and  select the ones where shift in
   * the gravity center must be done. tst=true means do not perform shift */
  for (i = 0; i < s->nclust; i++) {	/* loop1: */
    if ((s->punt[i + 1] - s->punt[i]) == 1)
      continue;
    tst = true;
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {	/* looptst : */
      l = s->clust[s->punt[i] + j];
      if (!s->again[l])
	goto loop1;
      if (s->goal[0] == 'c') {
	if (s->status[l][0] == 'c' && s->status[l][2] == 'u') {
	  tst = false;
	  break;
	}
      } else if ((s->status[l][0] == 'c' && s->status[l][2] == 'u')
		 || (s->status[l][0] == 'c' && s->status[l][2] == 'i')) {
	tst = false;
	break;
      }
    }				/* for */
    if (tst)
      goto loop1;

    /* Compute super center sc and super radius sr */
    mps_fsrad(s, i, sc, &sr);

    /* Check the relative width of the cluster
     * If it is greater than 1 do not shift
     * and set status(:1)='c' that means
     * keep iterating Aberth's step. */

    if (sr > cplx_mod(sc)) {
      for (j = s->punt[i]; j < s->punt[i + 1]; j++)
	s->status[s->clust[j]][0] = 'c';
      if (s->DOLOG)
	fprintf(s->logstr, "     FRESTART: cluster rel. large: skip to the next component\n");
      goto loop1;
    }
    
    /* Now check the Newton isolation of the cluster */

    for (k = 0; k < s->nclust; k++)
      if (k != i) {
	for (j = 0; j < s->punt[k + 1] - s->punt[k]; j++) {
	  cplx_sub(ctmp, sc, s->froot[s->clust[s->punt[k] + j]]);
	  rtmp = cplx_mod(ctmp);
	  rtmp1 = (sr + s->frad[s->clust[s->punt[k] + j]]) * 5 * s->n;
	  if (rtmp < rtmp1) {
	    for (jj = s->punt[i]; jj < s->punt[i + 1]; jj++)
	      s->status[s->clust[jj]][0] = 'c';
	    if (s->DOLOG) {
	      fprintf(s->logstr, "Cluster not Newton isolated:");
	      fprintf(s->logstr, "  skip to the next component\n");
	    }
	    goto loop1;
	  }
	}
      }
    /* Compute the coefficients of the derivative of p(x) having order
     * equal to the multiplicity of the cluster -1. */
    sum = 0.0;
    for (j = 0; j <= s->n; j++) {
      sum += cplx_mod(s->fpc[j]);
      cplx_set(s->fppc[j], s->fpc[j]);
    }
    for (j = 1; j < s->punt[i + 1] - s->punt[i]; j++) {
      for (k = 0; k <= s->n - j; k++)
	cplx_mul_d(s->fppc[k], s->fppc[k + 1], (double) (k + 1));
    }
    for (j = 0; j < s->n - (s->punt[i + 1] - s->punt[i]) + 2; j++)
      s->fap1[j] = cplx_mod(s->fppc[j]);

    /* Apply at most max_newt_it steps of Newton's iterations
     * to the above derivative starting from the super center
     * of the cluster. */
     
    cplx_set(g, sc);
    for (j = 0; j < s->max_newt_it; j++) {		/* loop_newt: */
      rad = 0.0;
      mps_fnewton(s, s->n - (s->punt[i + 1] - s->punt[i]) + 1, g,
		  &rad, corr, s->fppc, s->fap1, &cont);
      cplx_sub_eq(g, corr);
      if (!cont)
	break;
    }
    if (j == s->max_newt_it) {
      if (s->DOLOG)
	fprintf(s->logstr, "Exceeded maximum Newton iterations in frestart\n");
      return;
    }
    cplx_sub(ctmp, sc, g);
    if (cplx_mod(ctmp) > sr) {
      if (s->DOLOG)
	fprintf(s->logstr, "The gravity center falls outside the cluster\n");
      return;
    }
    /* Compute the coefficients of the shifted polynomial p(x+g)
     * and compute new starting approximations
     * First check if shift may cause overflow, in this case skip
     * the shift stage */

    if (s->n * log(cplx_mod(g)) + log(sum) > log(DBL_MAX))
      goto loop1;
    if (s->DOLOG)
      fprintf(s->logstr, "      FRESTART:  fshift\n");
    mps_fshift(s, s->punt[i + 1] - s->punt[i], i, sr, g, s->eps_out);
    rtmp = cplx_mod(g);
    rtmp *= DBL_EPSILON * 2;
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];
      /* Choose as new incl. radius 2*multiplicity*(radius of the circle) */
      s->frad[l] = 2 * (s->punt[i + 1] - s->punt[i]) * cplx_mod(s->froot[l]);
      cplx_add_eq(s->froot[l], g);
      if (s->frad[l] < rtmp)	/* DARIO* aggiunto 1/5/97 */
	s->frad[l] = rtmp;
    }
  loop1:;
  }
}

/*************************************************************
*                     SUBROUTINE DRESTART                    *
**************************************************************
 The program scans the existing clusters and  selects the ones 
 where shift in the gravity center must be done.
 Then computes the gravity center g, performs the shift of the variable
 and compute new starting approximations in the cluster.
 The components of the vector err are set to true (i.e., Aberth's
 iteration must be applied) if the cluster intersects the origin
 (in this case shift is not applied),
 or if new starting approximations have been selected.
 The gravity center g is choosen as a zero of the (m-1)-st derivative
 of the polynomial in the cluster, where m=mclust is the multiplicity
 of the cluster. 

 Shift in g is perfomed if
        status=(c*u) for goal=count
        status=(c*u) or status=(c*i) for goal=isolate/approximate
 To compute g, first compute the weighted mean (super center sc)
 of the approximations in the cluster, where the weight are the radii,
 then compute the radius (super radius sr) of the disk centered in the
 super center containing all the disks of the cluster.
 Apply few steps of Newton's iteration to the (m-1)-st derivative
 of the polynomial starting from the super center and obtain the point
 g where to shift the variable.
 If g is outside the super disk of center sc and radius sr output a warning
 message.
 ******************************************************************/
void
mps_drestart(mps_status* s)
{
  int i, k, j, l, jj;
  rdpe_t sr, rad, rtmp, rtmp1;
  cdpe_t sc, g, corr, ctmp;
  boolean tst, cont;

  /*  For user's polynomials skip the restart stage (not yet implemented) */
  if (s->data_type[0] == 'u')
    return;

  for (i = 0; i < s->nclust; i++) {	/* loop1: */
    if ((s->punt[i + 1] - s->punt[i]) == 1)
      continue;
    tst = true;
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {	/* looptst: */
      l = s->clust[s->punt[i] + j];
      if (!s->again[l])
	goto loop1;
      if (s->goal[0] == 'c') {
	if (s->status[l][0] == 'c' && s->status[l][2] == 'u') {
	  tst = false;
	  break;
	}
      } else if ((s->status[l][0] == 'c' && s->status[l][2] == 'u')
		 || (s->status[l][0] == 'c' && s->status[l][2] == 'i')) {
	tst = false;
	break;
      }
    }				/* for */
    if (tst)
      goto loop1;

    /* Compute super center sc and super radius sr */
    mps_dsrad(s, i, sc, sr);

    /* Check the relative width of the cluster
     * If it is greater than 1 do not shift
     * and set statu(:1)='c' that means
     * keep iterating Aberth's step. */
    cdpe_mod(rtmp, sc);
    if (rdpe_gt(sr, rtmp)) {
      for (j = s->punt[i]; j < s->punt[i + 1]; j++) {
	s->status[s->clust[j]][0] = 'c';
	/* err(clust[j])=true  */
      }
      if (s->DOLOG)
	fprintf(s->logstr, "     DRESTART: cluster rel. large: skip to the next component\n");
      goto loop1;
    }
    /* Now check the Newton isolation of the cluster */

    for (k = 0; k < s->nclust; k++)
      if (k != i) {
	for (j = 0; j < s->punt[k + 1] - s->punt[k]; j++) {
	  cdpe_sub(ctmp, sc, s->droot[s->clust[s->punt[k] + j]]);
	  cdpe_mod(rtmp, ctmp);
	  rdpe_add(rtmp1, sr, s->drad[s->clust[s->punt[k] + j]]);
	  rdpe_mul_eq_d(rtmp1, 2.0 * s->n);     
	  if (rdpe_lt(rtmp, rtmp1)) {
	    for (jj = s->punt[i]; jj < s->punt[i + 1]; jj++)
	      s->status[s->clust[jj]][0] = 'c';
	    if (s->DOLOG) {
	      fprintf(s->logstr, "cluster not Newton isolated:");
	      fprintf(s->logstr, " skip to the next component\n");
	    }
	    goto loop1;
	  }
	}
      }
      
    /* Compute the coefficients of the derivative of p(x) having order
     * equal to the multiplicity of the cluster -1. */
     
    for (j = 0; j <= s->n; j++)
      cdpe_set(s->dpc2[j], s->dpc[j]);
    for (j = 1; j < s->punt[i + 1] - s->punt[i]; j++) {
      for (k = 0; k <= s->n - j; k++)
	cdpe_mul_d(s->dpc2[k], s->dpc2[k + 1], (double) (k + 1));
    }
    for (j = 0; j < s->n - (s->punt[i + 1] - s->punt[i]) + 2; j++)
      cdpe_mod(s->dap1[j], s->dpc2[j]);

    /* Apply at most max_newt_it steps of Newton's iterations
     * to the above derivative starting from the super center
     * of the cluster. */
     
    cdpe_set(g, sc);
    for (j = 0; j < s->max_newt_it; j++) {		/* loop_newt: */
      rdpe_set(rad, rdpe_zero);
      mps_dnewton(s, s->n - (s->punt[i + 1] - s->punt[i]) + 1, g, rad,
		  corr, s->dpc2, s->dap1, &cont);
      cdpe_sub_eq(g, corr);
      if (!cont)
	break;
    }
    if (j == s->max_newt_it) {
      if (s->DOLOG)
	fprintf(s->logstr, "Exceeded maximum Newton iterations in frestart\n");
      return;
    }
    cdpe_sub(ctmp, sc, g);
    cdpe_mod(rtmp, ctmp);
    if (rdpe_gt(rtmp, sr)) {
      if (s->DOLOG)
	fprintf(s->logstr, "The gravity center falls outside the cluster\n");
      return;
    }
    /* Shift the variable and compute new approximations */
    if (s->DOLOG)
      fprintf(s->logstr, "      DRESTART:  dshift");
    mps_dshift(s, s->punt[i + 1] - s->punt[i], i, sr, g, s->eps_out);
    cdpe_mod(rtmp, g);
    rdpe_mul_eq_d(rtmp, DBL_EPSILON * 2);
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];

      /* Choose as new incl. radius 2*multiplicity*(radius of the circle) */
      cdpe_mod(s->drad[l], s->droot[l]);
      rdpe_mul_eq_d(s->drad[l], (double) (2 * (s->punt[i + 1] - s->punt[i])));
      cdpe_add_eq(s->droot[l], g);
      if (rdpe_lt(s->drad[l], rtmp))
	rdpe_set(s->drad[l], rtmp);
    }
  loop1:;
  }
}

/*************************************************************
*                     SUBROUTINE MRESTART                    *
*************************************************************/
void
mps_mrestart(mps_status* s)
{
  boolean tst, cont;
  int i, j, k, l, jj;
  rdpe_t sr, rad, rtmp, rtmp1, rtmp2;
  cdpe_t tmp;
  tmpf_t rea, srmp;
  tmpc_t sc, corr, temp;
  mpc_t g;
  
  /* For user's polynomials skip the restart stage (not yet implemented) */
  if (s->data_type[0] == 'u')
    return;

  tmpf_init2(rea, s->mpwp);
  tmpf_init2(srmp, s->mpwp);
  tmpc_init2(sc, s->mpwp);
  tmpc_init2(corr, s->mpwp);
  tmpc_init2(temp, s->mpwp);
  mpc_init2(g, s->mpwp);

  k = 0;
  for (i = 0; i < s->nclust; i++)
    k = MAX(k, s->punt[i + 1] - s->punt[i]);

  for (i = 0; i < s->nclust; i++) {	/* loop1: */
    if ((s->punt[i + 1] - s->punt[i]) == 1)
      continue;
    tst = true;
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {	/* looptst: */
      l = s->clust[s->punt[i] + j];
      if (!s->again[l])
	goto loop1;
      if (s->goal[0] == 'c') {
	if (s->status[l][0] == 'c' && s->status[l][2] == 'u') {
	  tst = false;
	  break;
	}
      } else if ((s->status[l][0] == 'c' && s->status[l][2] == 'u')
		 || (s->status[l][0] == 'c' && s->status[l][2] == 'i')) {
	tst = false;
	break;
      }
    }				/* for */

    if (tst)
      goto loop1;

    /* Compute super center sc and super radius sr */
    mps_msrad(s, i, sc, sr);
    
    if(s->DOLOG) {
      fprintf(s->logstr,"    MRESTART: clust=%d\n      sc=",i);
      mpc_out_str(s->logstr, 10, 10, sc);
      fprintf(s->logstr,"\n      sr=");
      rdpe_outln_str(s->logstr, sr);
    }
    
    /* Check the relative width of the cluster
     * If it is greater than 1 do not shift
     * and set status[:1)='c' that means
     * keep iterating Aberth's step. 
     * Check also the Newton-isolation of the cluster */

    mpc_get_cdpe(tmp, sc);
    cdpe_mod(rtmp, tmp);
    
    if(s->DOLOG){
      rdpe_div(rtmp2,sr,rtmp);
      fprintf(s->logstr,"      relative width=");
      rdpe_outln_str(s->logstr, rtmp2);
    }
    
    if (rdpe_gt(sr, rtmp)) {
      for (j = s->punt[i]; j < s->punt[i + 1]; j++)
	s->status[s->clust[j]][0] = 'c';
      if (s->DOLOG)
	fprintf(s->logstr, "    MRESTART: cluster %d relat. large: skip to the next component\n",i);
      goto loop1;
    }
    
    /* Now check the Newton isolation of the cluster */
    rdpe_set(rtmp2, rdpe_zero);
    for (k = 0; k < s->nclust; k++){
      if (k != i)
	for (j = 0; j < s->punt[k + 1] - s->punt[k]; j++) {
	  mpc_sub(temp, sc, s->mroot[s->clust[s->punt[k] + j]]);
	  mpc_get_cdpe(tmp, temp);
	  cdpe_mod(rtmp, tmp);
          rdpe_sub_eq(rtmp,s->drad[s->clust[s->punt[k] + j]]);
          rdpe_sub_eq(rtmp,sr);
	  rdpe_inv_eq(rtmp);
	  rdpe_add_eq(rtmp2,rtmp);
	}
    }
    rdpe_mul_eq(rtmp2,sr);
    rdpe_set_d(rtmp1, 0.3);

    if (rdpe_gt(rtmp2, rtmp1)) {
      for (jj = s->punt[i]; jj < s->punt[i + 1]; jj++)
	s->status[s->clust[jj]][0] = 'c';
      if (s->DOLOG) {
	fprintf(s->logstr, "    MRESTART: Cluster not Newton isolated:");
	fprintf(s->logstr, "              skip to the next component\n");
      }
      goto loop1;
    }
 
    if(s->DOLOG){
      fprintf(s->logstr,"    MRESTART: Approximations of cluster %d\n", i);
      for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
	l = s->clust[s->punt[i] + j];
	mpc_get_cdpe(tmp, s->mroot[l]);
	cdpe_out_str(s->logstr, tmp);
	fprintf(s->logstr,"  rad=");
        rdpe_outln_str(s->logstr,s->drad[l]);
      }
    }


    /* Compute the coefficients of the derivative of p(x) having order
     * equal to the multiplicity of the cluster -1. */

    for (j = 0; j <= s->n; j++)
      mpc_set(s->mfpc1[j], s->mfpc[j]);
    for (j = 1; j < s->punt[i + 1] - s->punt[i]; j++) {
      for (k = 0; k <= s->n - j; k++)
	mpc_mul_ui(s->mfpc1[k], s->mfpc1[k + 1], k + 1);
    }
    for (j = 0; j < s->n - (s->punt[i + 1] - s->punt[i]) + 2; j++) {
      mpc_get_cdpe(tmp, s->mfpc1[j]);
      cdpe_mod(s->dap1[j], tmp);
    }

    /* create the vectors needed if the polynomial is sparse */

    if (s->data_type[0] == 's') {
      for (j = 0; j < s->n - (s->punt[i + 1] - s->punt[i]) + 2; j++) {
	if (rdpe_ne(s->dap1[j], rdpe_zero))
	  s->spar1[j] = true;
	else
	  s->spar1[j] = false;
      }
      for (j = 0; j < s->n - (s->punt[i + 1] - s->punt[i]) + 1; j++)
	mpc_mul_ui(s->mfppc1[j], s->mfpc1[j + 1], j + 1);
    }
    /* Apply at most max_newt_it steps of Newton's iterations
     * to the above derivative starting from the super center
     * of the cluster. */
    mpc_set(g, sc);

    if (s->DOLOG) {
      fprintf(s->logstr, "    MRESTART: g before newton=");
      mpc_outln_str(s->logstr, 10, 30, g);
    }
    for (j = 0; j < s->max_newt_it; j++) {		/* loop_newt: */
      rdpe_set(rad, rdpe_zero);
      mps_mnewton(s, s->n - (s->punt[i + 1] - s->punt[i]) + 1, g, rad, corr, s->mfpc1,
		  s->mfppc1, s->dap1, s->spar1, &cont);
      if (cont) {
	mpc_sub_eq(g, corr);
	if (s->DOLOG) {
	  fprintf(s->logstr, "    MRESTART: radius=");
	  rdpe_outln_str(s->logstr, rad);
	  fprintf(s->logstr, "    MRESTART: at iteration %d, g=", j);
	  mpc_outln_str(s->logstr, 10, 100, g);
	}
      } else
	break;
    }
    if (s->DOLOG)
      fprintf(s->logstr, "    MRESTART: performed %d Newton iter\n", j);
    if (j == s->max_newt_it) {
      if (s->DOLOG)
	fprintf(s->logstr, "Exceeded maximum Newton iterations in mrestart\n");
        goto loop1;
    }
    mpc_sub(temp, sc, g);
    mpc_get_cdpe(tmp, temp);
    cdpe_mod(rtmp, tmp);
    if (rdpe_gt(rtmp, sr)) {
      if (s->DOLOG)
	fprintf(s->logstr, "The gravity center falls outside the cluster\n");
      goto loop1;
    }
    
    /* shift the variable and compute new approximations */
    
    if (s->DOLOG)
      fprintf(s->logstr, "      MRESTART: call mshift\n");
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];
      mpc_get_cdpe(s->droot[l], s->mroot[l]);
    }
/*#D perform shift only if the new computed sr is smaller than old*0.25 */
    rdpe_mul_d(rtmp, sr, 0.25);
/*#D AGO99 Factors: 0.1 (MPS2.0), 0.5 (GIUGN98) */
    mps_mshift(s, s->punt[i + 1] - s->punt[i], i, sr, g);
    if(rdpe_lt(sr, rtmp)){ /* Perform shift only if the new clust is smaller */
      mpc_get_cdpe(tmp, g);
      cdpe_mod(rtmp, tmp);
      rdpe_mul_eq(rtmp, s->mp_epsilon);
      rdpe_mul_eq_d(rtmp, 2);
      
      for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
	l = s->clust[s->punt[i] + j];
	mpc_set_cdpe(s->mroot[l], s->droot[l]);
	mpc_add_eq(s->mroot[l], g);
	cdpe_mod(rtmp1, s->droot[l]);
	rdpe_mul_d(s->drad[l], rtmp1, 2.0 * (s->punt[i + 1] - s->punt[i]));
	if (rdpe_lt(s->drad[l], rtmp))	
	  rdpe_set(s->drad[l], rtmp);
      }
    } else { 

    if(s->DOLOG) {
       fprintf(s->logstr,"    MRESTART: DO NOT PERFORM RESTART\n");
       fprintf(s->logstr,"    MRESTART: new radius of the cluster is larger\n");
     }
     
     goto loop1;
    }
  loop1:;
  }

  mpc_clear(g);
  tmpc_clear(temp);
  tmpc_clear(corr);
  tmpc_clear(sc);
  tmpf_clear(srmp);
  tmpf_clear(rea);
}

/**************************************************************
*                      SUBROUTINE FSHIFT                      *
***************************************************************
 This routine computes the first m+1 coefficients of the shifted 
 polynomial p(x+g), by performing m+1 Horner divisions.
 Then it computes the new starting approximations for the i-th
 cluster for i=i_clust by ing fstart and by updating root.
 The status vector is changed into 'o' for the components that
 belong to a cluster with relative radius less than eps.
 The status vector is changed into 'x' for the components that
 cannot be represented as float.
 **************************************************************/
void
mps_fshift(mps_status* s, int m, int i_clust, double clust_rad, cplx_t g, rdpe_t eps)
{
  int i, j;
  double prec, ag;
  cplx_t t;

  /* Perform divisions */

  prec = DBL_EPSILON;
  ag = cplx_mod(g);
  for (i = 0; i <= s->n; i++)
    cplx_set(s->fppc1[i], s->fpc[i]);
  for (i = 0; i <= m; i++) {
    cplx_set(t, s->fppc1[s->n]);
    for (j = s->n - 1; j >= i; j--) {
      cplx_mul_eq(t, g);
      cplx_add_eq(t, s->fppc1[j]);
      cplx_set(s->fppc1[j], t);
    }
    cplx_set(s->fppc[i], t);
  }

  /* start */
  for (i = 0; i <= m; i++)
    s->fap1[i] = cplx_mod(s->fppc[i]);

  mps_fstart(s, m, i_clust, clust_rad, ag, eps, s->fap1);
}

/***********************************************************
*                   SUBROUTINE DSHIFT                      *
***********************************************************/
void
mps_dshift(mps_status* s, int m, int i_clust, rdpe_t clust_rad,
       cdpe_t g, rdpe_t eps)
{
  int i, j;
  rdpe_t prec, ag;
  cdpe_t t;

  rdpe_set_d(prec, DBL_EPSILON);
  cdpe_mod(ag, g);
  for (i = 0; i <= s->n; i++)
    cdpe_set(s->dpc1[i], s->dpc[i]);
  for (i = 0; i <= m; i++) {
    cdpe_set(t, s->dpc1[s->n]);
    for (j = s->n - 1; j >= i; j--) {
      cdpe_mul_eq(t, g);
      cdpe_add_eq(t, s->dpc1[j]);
      cdpe_set(s->dpc1[j], t);
    }
    cdpe_set(s->dpc2[i], t);
  }

  /* start */
  for (i = 0; i <= m; i++)
    cdpe_mod(s->dap1[i], s->dpc2[i]);

  mps_dstart(s, m, i_clust, clust_rad, ag, eps, s->dap1);
}

/*******************************************************
*              SUBROUTINE MSHIFT                       *
*******************************************************/
void
mps_mshift(mps_status* s, int m, int i_clust, rdpe_t clust_rad, mpc_t g)
{
  int i, j, k;
  long int mpwp_temp, mpwp_max;
  rdpe_t ag, ap, abp, as, mp_ep;
  cdpe_t abd;
  mpc_t t;

  mpc_init2(t, s->mpwp);

  /* Perform divisions
   * In the mp version of the shift stage the computation
   * is performed with increasing levels of working precision
   * until the coefficients of the shifted polynomial have at
   * least one correct bit. */

  rdpe_set(mp_ep, s->mp_epsilon);
  mpc_get_cdpe(abd, g);
  cdpe_mod(ag, abd);
  for (i = 0; i <= s->n; i++)
    mpc_set(s->mfpc1[i], s->mfpc[i]);
  rdpe_set(as, rdpe_zero);
  rdpe_set(ap, rdpe_one);
  mpc_set_ui(t, 0, 0);
  k = 0;

  /* store the current working precision mpnw into mpnw_tmp */
  mpwp_temp = s->mpwp;
  mpwp_max = s->mpwp;

  do {				/* loop */
    mpc_set(t, s->mfpc1[s->n]);
    mpc_get_cdpe(abd, s->mfpc[s->n]);
    cdpe_mod(ap, abd);
    for (j = s->n - 1; j >= 0; j--) {
      mpc_get_cdpe(abd, s->mfpc[j]);
      cdpe_mod(abp, abd);
      rdpe_mul_eq(ap, ag);
      rdpe_mul_eq_d(abp, (double) j);
      rdpe_add_eq(ap, abp);
      mpc_mul_eq(t, g);
      mpc_add_eq(t, s->mfpc1[j]);
      mpc_set(s->mfpc1[j], t);
    }

    mpc_set(s->mfppc1[0], t);
    mpc_get_cdpe(abd, t);
    cdpe_mod(as, abd);
    rdpe_mul_eq(ap, mp_ep);
    rdpe_mul_eq_d(ap, 4.0 * (s->n + 1));
    k++;

    if (rdpe_lt(as, ap)) {
      mpwp_temp += s->mpwp;

      if (mpwp_temp > mpwp_max || mpwp_temp > s->prec_out * m * 2) { 
	if (s->DOLOG)
	  fprintf(s->logstr, "Reached the maximum allowed precision in mshift\n");
	break;
      }
      rdpe_set_2dl(mp_ep, 1.0, 1 - mpwp_temp);
      mps_raisetemp(s, mpwp_temp);
      mpc_set_prec(t, (unsigned long int) mpwp_temp);
      mpc_set_prec(g, (unsigned long int) mpwp_temp);
      if (mpwp_max < mpwp_temp) 
	mpwp_max = mpwp_temp;

      for (j = 0; j <= s->n; j++)
	mpc_set(s->mfpc1[j], s->mfpc[j]);
    }
  } while (rdpe_lt(as, ap) && (k <= m));	/* loop */

  for (i = 1; i <= m; i++) {
    mpwp_temp = MAX(mpwp_temp - s->mpwp, s->mpwp);
    mps_raisetemp_raw(s, mpwp_temp);
    mpc_set_prec_raw(t, (unsigned long int) mpwp_temp);
    mpc_set_prec_raw(g, (unsigned long int) mpwp_temp);
    mpc_set(t, s->mfpc1[s->n]);

    for (j = s->n - 1; j >= i; j--) {
      mpc_mul_eq(t, g);
      mpc_add_eq(t, s->mfpc1[j]);
      mpc_set(s->mfpc1[j], t);
    }
    mpc_set(s->mfppc1[i], t);

  }
  /*
    raisetemp_raw(mpwp);
    mpc_set_prec_raw(s, (unsigned long int) mpwp);
    mpc_set_prec_raw(g, (unsigned long int) mpwp);
  
   segue alternativa
  */
  mps_raisetemp_raw(s, mpwp_max);
  mpc_set_prec_raw(t, (unsigned long int) mpwp_max);
  mpc_set_prec_raw(g, (unsigned long int) mpwp_max);
  mps_raisetemp(s, s->mpwp);
  mpc_set_prec(t, (unsigned long int) s->mpwp);
  mpc_set_prec(g, (unsigned long int) s->mpwp);

  if (rdpe_lt(as, ap)) {
    for (j = 0; j < m; j++)
      rdpe_set(s->dap1[j], ap);
    mpc_get_cdpe(abd, s->mfppc1[m]);
    cdpe_mod(s->dap1[m], abd);
  } else
    for (i = 0; i <= m; i++) {
      mpc_get_cdpe(abd, s->mfppc1[i]);
      cdpe_mod(s->dap1[i], abd);
    }

  mps_mstart(s, m, i_clust, clust_rad, ag, s->dap1);

  mpc_clear(t);
}

/**************************************************************
 *               SUBROUTINE RAISETEMP                         *
 *************************************************************/
void
mps_raisetemp(mps_status* s, unsigned long int digits)
{
  int i;

  for (i = 0; i <= s->n; i++) {
    mpc_set_prec(s->mfpc1[i], digits);
    mpc_set_prec(s->mfppc1[i], digits);
  }
}

/**************************************************************
 *               SUBROUTINE RAISETEMP_RAW                     *
 *************************************************************/
void
mps_raisetemp_raw(mps_status* s, unsigned long int digits)
{
  int i;

  for (i = 0; i <= s->n; i++) {
    mpc_set_prec_raw(s->mfpc1[i], digits);
    mpc_set_prec_raw(s->mfppc1[i], digits);
  }
}

/**************************************************************
 *               SUBROUTINE MNEWTIS                           *
 *************************************************************/
void 
mps_mnewtis(mps_status* s)
{
  boolean tst;
  int i, j, k, l, jj;
  rdpe_t sr, rtmp, rtmp1;
  cdpe_t tmp;
  tmpf_t rea, srmp;
  tmpc_t sc, temp;
  rdpe_t rtmp2; 

  /* For user's polynomials skip the restart stage (not yet implemented) */
  if (s->data_type[0] == 'u')
    return;
  tmpf_init2(rea, s->mpwp);
  tmpf_init2(srmp, s->mpwp);
  tmpc_init2(sc, s->mpwp);
  tmpc_init2(temp, s->mpwp);

  k = 0;
  for (i = 0; i < s->nclust; i++)
    k = MAX(k, s->punt[i + 1] - s->punt[i]);

  for (i = 0; i < s->nclust; i++) {	/* loop1: */

    if ((s->punt[i + 1] - s->punt[i]) == 1)
      continue;
    tst = true;
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {	/* looptst: */
      l = s->clust[s->punt[i] + j];
      if (!s->again[l])
	goto loop1;
      if (s->goal[0] == 'c') {
	if (s->status[l][0] == 'c' && s->status[l][2] == 'u') {
	  tst = false;
	  break;
	}
      } else if ((s->status[l][0] == 'c' && s->status[l][2] == 'u')
		 || (s->status[l][0] == 'c' && s->status[l][2] == 'i')) {
	tst = false;
	break;
      }
    }				/* for */
    if (tst)
      goto loop1;

    /* Compute super center sc and super radius sr */
    mpf_set_ui(srmp, 0);
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];
      mpf_set_rdpe(rea, s->drad[l]);
      mpf_add(srmp, srmp, rea);
    }
    mpc_set_ui(sc, 0, 0);
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];
      mpf_set_rdpe(rea, s->drad[l]);
      mpc_mul_f(temp, s->mroot[l], rea);
      mpc_add_eq(sc, temp);
    }
    mpc_div_eq_f(sc, srmp);
    rdpe_set(sr, rdpe_zero);
    for (j = 0; j < s->punt[i + 1] - s->punt[i]; j++) {
      l = s->clust[s->punt[i] + j];
      mpc_sub(temp, sc, s->mroot[l]);
      mpc_get_cdpe(tmp, temp);
      cdpe_mod(rtmp, tmp);
      rdpe_add_eq(rtmp, s->drad[l]);
      if (rdpe_lt(sr, rtmp))
	rdpe_set(sr, rtmp);
    }

    /* Check the relative width of the cluster
     * If it is greater than 1 do not shift
     * and set status[:1)='c' that means
     * keep iterating Aberth's step. 
     * Check also the Newton-isolation of the cluster */

    mpc_get_cdpe(tmp, sc);
    cdpe_mod(rtmp, tmp);
    rdpe_div(rtmp2,sr,rtmp);
    if (rdpe_gt(sr, rtmp)) {
      for (j = s->punt[i]; j < s->punt[i + 1]; j++)
	s->status[s->clust[j]][0] = 'c';
      if (s->DOLOG)
	fprintf(s->logstr, "   MNEWTIS cluster %d relat. large: "
		"skip to the next component\n", i);
      goto loop1;
    }
    
    /* Now check the Newton isolation of the cluster */
    rdpe_set(rtmp2, rdpe_zero);        
    for (k = 0; k < s->nclust; k++){
      if (k != i)
	for (j = 0; j < s->punt[k + 1] - s->punt[k]; j++) {
	  mpc_sub(temp, sc, s->mroot[s->clust[s->punt[k] + j]]);
	  mpc_get_cdpe(tmp, temp);
	  cdpe_mod(rtmp, tmp);
          rdpe_sub_eq(rtmp,s->drad[s->clust[s->punt[k] + j]]);
          rdpe_sub_eq(rtmp,sr);
	  rdpe_inv_eq(rtmp);
	  rdpe_add_eq(rtmp2,rtmp);
	}
    }
    rdpe_mul_eq(rtmp2,sr);
    rdpe_set_d(rtmp1,0.3);

    if (rdpe_gt(rtmp2, rtmp1)) {
      for (jj = s->punt[i]; jj < s->punt[i + 1]; jj++)
	s->status[s->clust[jj]][0] = 'c';
      if (s->DOLOG) {
	fprintf(s->logstr, "   MNEWTIS Cluster not Newton isolated:");
	fprintf(s->logstr, "           skip to the next component\n");
      }
      goto loop1;
    } 
    s->newtis=1;

  loop1:;
}
  
  tmpc_clear(temp);
  tmpc_clear(sc);
  tmpf_clear(srmp);
  tmpf_clear(rea);
}