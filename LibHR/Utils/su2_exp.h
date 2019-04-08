/***************************************************************************\
* Copyright (c) 2008, Agostino Patella                                      *   
* All rights reserved.                                                      * 
\***************************************************************************/

#include "suN.h"
#include "utils.h"
#include "suN_repr_func.h" // for ExpX_taylor
#include <math.h>

#if (NG!=2) 
#error : Mismatch between NG and ExpX!
#endif

/*
*  U = (1+i*y.sigma/4)(1-i*y.sigma/4)^{-1}
*  U = u[0] + i*u.sigma/4
*/
static void YtoU(double* u, double *y)
{
   double y2 = y[0]*y[0] + y[1]*y[1] +y[2]*y[2];
   double detY = 1.0 + y2/16.;
   u[0] = (1.0 - y2/16.)/detY;
   u[1] = y[0]/(2.*detY);
   u[2] = y[1]/(2.*detY);
   u[3] = y[2]/(2.*detY);
}


/*
*  Applica la rotazione di SU(2) definita da
*  U = s[0] + i*s.sigma/4
*  al vettore (v1,v2)
*/
static void su2_rotate(double *s,complex *v1,complex *v2)
{
   complex z1, z2;
   z1.re=
      s[0]*(*v1).re-s[1]*(*v2).im+s[2]*(*v2).re-s[3]*(*v1).im;
   z1.im=
      s[0]*(*v1).im+s[1]*(*v2).re+s[2]*(*v2).im+s[3]*(*v1).re;
   z2.re=
      s[0]*(*v2).re-s[1]*(*v1).im-s[2]*(*v1).re+s[3]*(*v2).im;
   z2.im=
      s[0]*(*v2).im+s[1]*(*v1).re-s[2]*(*v1).im-s[3]*(*v2).re;
   (*v1)=z1;
   (*v2)=z2;
}

static inline void ExpX_taylor( double dt, suNg_algebra_vector *h, suNg *r){
    suNg ufinal, addendum,dtiT;
    _suNg_unit(ufinal);// u = 1
    suNg iT;
    _fund_algebra_represent(iT,*h);
    _suNg_mul(dtiT,dt,iT); 
    addendum = dtiT; // for order = 1
    _suNg_add_assign(ufinal,addendum);// u = 1+ dt*iT
    for(int order = 2; order != 10;++order){
        suNg tmp;
        // addendum[order] = 1.0/order * dtiT * addendum[order-1]
        _suNg_times_suNg(tmp,dtiT,addendum);
        _suNg_mul(addendum,1.0/order,tmp); 
        _suNg_add_assign(ufinal,addendum);// u += (dt*iT)^order/order!
    } 
    suNg result;
    _suNg_times_suNg(result,ufinal,*r);
    *r = result;
}


/*
*  Approssima
*  V = exp(sum(A) iT(A)*h(A)*dt)
*  con una matrice unitaria secondo l'algoritmo di Luscher
*  e sostituisce
*  V.U -> U
*/
#if  defined(GAUGE_SON) || defined(GAUGE_SPN)
void ExpX(double dt, suNg_algebra_vector *h, suNg *r)
#else
void ExpX(double dt, suNg_algebra_vector *h, suNg *u)
#endif
{
#ifdef WITH_QUATERNIONS
suNg v_tmp, u_tmp;

u_tmp=*u;
_suNg_exp(dt,*h,v_tmp);
_suNg_times_suNg(*u,v_tmp,u_tmp);
#else //WITH_QUATERNIONS 

	double y[3];
	double s[1][4];
	y[0] = +5.000000000000000e-01*(*h).c[1]*(dt);
	y[1] = -5.000000000000000e-01*(*h).c[0]*(dt);
	y[2] = +5.000000000000000e-01*(*h).c[2]*(dt);
	YtoU(s[0],y);
	su2_rotate(s[0],&((*u).c[0]),&((*u).c[2]));
	su2_rotate(s[0],&((*u).c[1]),&((*u).c[3]));
	su2_rotate(s[0],&((*u).c[0]),&((*u).c[2]));
	su2_rotate(s[0],&((*u).c[1]),&((*u).c[3]));


#endif //WITH_QUATERNIONS
}



#define XG(m,a,b) ((m)+(a)*NG+(b))
#define XA(h,a) ((h)+(a))
#define TI(a,b) ((a)*NG+(b)-1)


void ExpX2(double dt, suNg_algebra_vector *h, suNg *u)
{
#ifdef WITH_QUATERNIONS

suNg v_tmp, u_tmp;

u_tmp=*u;
_suNg_exp(dt,*h,v_tmp);
_suNg_times_suNg(*u,v_tmp,u_tmp);

#else //WITH_QUATERNIONS 

	int i, j, k, n;
	double y[3];
	double d[NG];
	double s[NG*(NG-1)/2][4];
	double tmp;
	double *hf = (double*)h;
	complex *uf = (complex*)u;
	
	d[0] = 0.0;
	for(i = 1; i < NG; i++) {
		tmp = sqrt( 2./(i*(i+1)) ) * (*XA(hf,i));
		d[i] = -i *tmp;
		for(j = 0; j < i; j++)
			d[j] += tmp;
	}
	
	k = 0;
	for(j = 1; j<NG; j++) /* Aggiunto il j< testare. Claudio */
	for(i = 0; i < j; i++) {
		y[0] = dt * (*XA(hf,TI(j,i)));
		y[1] = -dt * (*XA(hf,TI(i,j)));
		y[2] = dt * (d[i]-d[j]) / NG;
		YtoU(s[k],y);
		for(n = 0; n < NG; n++)
			su2_rotate(s[k],XG(uf,i,n),XG(uf,j,n));
		k++;
	}

	k = NG*(NG-1)/2 - 1;
	for(j = NG-1; j >= 1; j--)
	for(i = j-1; i >= 0; i--) {
		for(n = 0; n < NG; n++)
			su2_rotate(s[k],XG(uf,i,n),XG(uf,j,n));
		k--;
	}

#endif //WITH_QUATERNIONS

}



