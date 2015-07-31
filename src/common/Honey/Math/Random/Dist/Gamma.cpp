// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Honey/Math/Random/Dist/Gamma.h"
#include "Honey/Math/Random/Random.h"

namespace honey
{


template<class Real>
Real Gamma_<Real>::next() const
{
    //Algorithm and code adapted from "A simple method for generating gamma variables", G. Marsaglia, 2000
    Double ad = a, bd = b;

    if (a < 1)
        return Alge_d::pow(Uniform::nextStd(getGen()), 1/ad) * Gamma(getGen(), 1+ad, bd).next();

    Double v;
    Double d = ad - 1/3.;
    Double c = (1/3.) / Alge_d::sqrt(d);

    for(;;)
    {
        Double x;
        do
        {
            x = Gaussian(getGen()).next();
            v = 1 + c*x;
        }
        while (v <= 0);

        v = v*v*v;
        Double u = Uniform::nextStd(getGen());
        Double xSqr = Alge_d::sqr(x);

        if (u < 1 - 0.0331*Alge_d::sqr(xSqr)) 
            break;

        if (Alge_d::log(u) < 0.5*xSqr + d*(1 - v + Alge_d::log(v)))
            break;
    }

    return bd * d * v;
}

template<class Real>
Real Gamma_<Real>::pdf(Real x) const
{
    Double xd = x, ad = a, bd = b;

    if (x < 0)
        return 0;
    else if (x == 0)
    {
        if (a == 1)
            return 1/bd;
        else
            return 0;
    }
    else if (a == 1)
        return Alge_d::exp(-xd/bd) / bd;
    else 
        return Alge_d::exp((ad - 1) * Alge_d::log(xd/bd) - xd/bd - GammaFunc::gammaLn(ad)) / bd;
}


//==============================================================================================================
// Class to evaluate gamma and log of gamma
//==============================================================================================================

template<class Real>
Real GammaFunc_<Real>::series(Real z)
{
   static const Real n0 = 1.846356774800897077637235e6L;
   static const Real n1 = 1.759131712935803984850945e6L;
   static const Real n2 = 7.542124083269936035445648e5L;
   static const Real n3 = 1.916219552338091802379555e5L;
   static const Real n4 = 3.194965924862382624981206e4L;
   static const Real n5 = 3.652838209061050933543152e3L;
   static const Real n6 = 2.900228320454639341680104e2L;
   static const Real n7 = 1.578981962865355560648172e1L;
   static const Real n8 = 0.564145967416346085128381L;
   static const Real n9 = 0.119443368011180931171494e-1L;
   static const Real n10 = 0.113800747608906017093789e-3L;
   static const Real d0 = 3628800;
   static const Real d1 = 10628640;
   static const Real d2 = 12753576;
   static const Real d3 = 8409500;
   static const Real d4 = 3416930;
   static const Real d5 = 902055;
   static const Real d6 = 157773;
   static const Real d7 = 18150;
   static const Real d8 = 1320;
   static const Real d9 = 55;
   static const Real d10 = 1;
   
   if (z < 10.0)
   {
      const Real num = n0 + z * (n1 + z * (n2 + z * (n3 + z * (n4 + z * (n5 
       + z * (n6 + z * (n7 + z * (n8 + z * (n9 + z * n10)))))))));
      const Real den = d0 + z * (d1 + z * (d2 + z * (d3 + z * (d4 + z * (d5 
       + z * (d6 + z * (d7 + z * (d8 + z * (d9 + z)))))))));
      return num / den;
   }
   else
   {
      Real r = 1.0L / z; 
      const Real num = n10 + r * (n9 + r * (n8 + r * (n7 + r * (n6 + r * (n5 
       + r * (n4 + r * (n3 + r * (n2 + r * (n1 + r * n0)))))))));
      const Real den = d10 + r * (d9 + r * (d8 + r * (d7 + r * (d6 + r * (d5 
       + r * (d4 + r * (d3 + r * (d2 + r * (d1 + r * d0)))))))));
      return num / den;
   }
}

// version for z > 100
template<class Real>
Real GammaFunc_<Real>::asymp(Real z)
{
   Real z2 = z * z;
   return
      (z - 0.5) * Alge::log(z) - z + 0.9189385332046727417L
      + (1.0L/12.0L + (-1.0L/360.0L + (1.0L/1260.0L - 1.0L/1680.0L/z2)/z2)/z2)/z;
}

// version for z > 1 - Lanczos method
template<class Real>
Real GammaFunc_<Real>::lanczos(Real z)
{
   Real zg5 = gValue() + z - 0.5;
   return Alge::log(series(z - 1.0)) + (z - 0.5) * (Alge::log(zg5) - 1.0);
}

//for z near 1
template<class Real>
Real GammaFunc_<Real>::near1(Real z)
{
   Real z1 = z - 1;
   return
        z1 * (-0.57721566490153286061L
      + z1 * ( 1.64493406684822643647L / 2
      + z1 * (-1.20205690315959428540L / 3
      + z1 * ( 1.08232323371113819152L / 4
      + z1 * (-1.03692775514336992633L / 5
      + z1 * ( 1.01734306198444913971L / 6
      + z1 * (-1.00834927738192282684L / 7
      + z1 * ( 1.00407735619794433938L / 8
      + z1 * (-1.00200838282608221442L / 9
      + z1 * ( 1.00099457512781808534L / 10
      + z1 * (-1.00049418860411946456L / 11
      + z1 * ( 1.00024608655330804830L / 12
      + z1 * (-1.00012271334757848915L / 13
      + z1 * ( 1.00006124813505870483L / 14
      + z1 * (-1.00003058823630702049L / 15
      + z1 * ( 1.00001528225940865187L / 16
      + z1 * (-1.00000763719763789976L / 17
      + z1 * ( 1.00000381729326499984L / 18
      + z1 * (-1.00000190821271655394L / 19
      + z1 *   1.00000095396203387280L / 20 )))))))))))))))))));
}

//for z near 2
template<class Real>
Real GammaFunc_<Real>::near2(Real z)
{
   Real z1 = z - 2;
   return
        z1 * (1 - 0.57721566490153286061L
      + z1 * ( 0.64493406684822643647L / 2
      + z1 * (-0.20205690315959428540L / 3
      + z1 * ( 0.08232323371113819152L / 4
      + z1 * (-0.03692775514336992633L / 5
      + z1 * ( 0.01734306198444913971L / 6
      + z1 * (-0.00834927738192282684L / 7
      + z1 * ( 0.00407735619794433938L / 8
      + z1 * (-0.00200838282608221442L / 9
      + z1 * ( 0.00099457512781808534L / 10
      + z1 * (-0.00049418860411946456L / 11
      + z1 * ( 0.00024608655330804830L / 12
      + z1 * (-0.00012271334757848915L / 13
      + z1 * ( 0.00006124813505870483L / 14
      + z1 * (-0.00003058823630702049L / 15)))))))))))))));
}

// version for z > 0.9
template<class Real>
Real GammaFunc_<Real>::gt1(Real z)
{
   if (z >= 100) return asymp(z);
   if (z >= 2.1) return lanczos(z);
   if (z >= 1.9) return near2(z);   
   if (z >= 1.1) return lanczos(z);
   if (z >= 0.9) return near1(z);
   error_("Argument less than 1");
   return 0;
}

template<class Real>
Real GammaFunc_<Real>::gammaLn(Real z, int& sign)
{
   if (z >= 2.1) { sign = 1; return gt1(z); }
   if (z >= 1.9) { sign = 1; return near2(z); }   
   if (z >= 1.1) { sign = 1; return gt1(z); }
   if (z >= 0.9) { sign = 1; return near1(z); }
   else                           // Use reflection formula
   {
      Real cz = Alge::ceil(z); Real cz2 = cz / 2.0;
      sign = Alge::abs(Alge::ceil(cz2) - cz2) > 0.25 ? 1 : -1;
      Real fz = cz - z;
      if (fz == 0) error_("Non-positive integer argument");
      Real piz = 3.14159265358979323846 * (1.0 - z);
      return
         Alge::log(piz / Trig::sin(3.14159265358979323846 * fz)) - gt1(2.0 - z);
   }
}

template<class Real>
Real GammaFunc_<Real>::gammaLn(Real z)
{
   int sign;
   Real lg = gammaLn(z, sign);
   if (sign < 0) error_("Negative gamma value");
   return lg;
}

template<class Real>
const int GammaFunc_<Real>::factorialTableSize = 171;

//Factorial table accurate to 30 decimal digits
template<class Real>
const typename GammaFunc_<Real>::Double GammaFunc_<Real>::factorialTable[factorialTableSize] = {
    1.e0L                                   ,       //0  
    1.e0L                                   ,       //1  
    2.e0L                                   ,       //2  
    6.e0L                                   ,       //3  
    2.4e1L                                  ,       //4  
    1.2e2L                                  ,       //5  
    7.2e2L                                  ,       //6  
    5.04e3L                                 ,       //7  
    4.032e4L                                ,       //8                            
    3.6288e5L                               ,       //9
    3.6288e6L                               ,       //10 
    3.99168e7L                              ,       //11 
    4.790016e8L                             ,       //12                                          
    6.2270208e9L                            ,       //13 
    8.71782912e10L                          ,       //14 
    1.307674368e12L                         ,       //15 
    2.0922789888e13L                        ,       //16 
    3.55687428096e14L                       ,       //17 
    6.402373705728e15L                      ,       //18 
    1.21645100408832e17L                    ,       //19 
    2.43290200817664e18L                    ,       //20 
    5.109094217170944e19L                   ,       //21 
    1.12400072777760768e21L                 ,       //22 
    2.585201673888497664e22L                ,       //23 
    6.2044840173323943936e23L               ,       //24 
    1.5511210043330985984e25L               ,       //25 
    4.03291461126605635584e26L              ,       //26 
    1.0888869450418352160768e28L            ,       //27 
    3.04888344611713860501504e29L           ,       //28 
    8.841761993739701954543616e30L          ,       //29 
    2.6525285981219105863630848e32L         ,       //30 
    8.22283865417792281772556288e33L        ,       //31 
    2.6313083693369353016721801216e35L      ,       //32 
    8.68331761881188649551819440128e36L     ,       //33 
    2.95232799039604140847618609644e38L     ,       //34 
    1.03331479663861449296666513375e40L     ,       //35 
    3.71993326789901217467999448151e41L     ,       //36 
    1.37637530912263450463159795816e43L     ,       //37 
    5.23022617466601111760007224100e44L     ,       //38 
    2.03978820811974433586402817399e46L     ,       //39 
    8.15915283247897734345611269600e47L     ,       //40 
    3.34525266131638071081700620534e49L     ,       //41 
    1.40500611775287989854314260624e51L     ,       //42 
    6.04152630633738356373551320685e52L     ,       //43 
    2.65827157478844876804362581101e54L     ,       //44 
    1.19622220865480194561963161496e56L     ,       //45 
    5.50262215981208894985030542880e57L     ,       //46 
    2.58623241511168180642964355154e59L     ,       //47 
    1.24139155925360726708622890474e61L     ,       //48 
    6.08281864034267560872252163321e62L     ,       //49 
    3.04140932017133780436126081661e64L     ,       //50 
    1.55111875328738228022424301647e66L     ,       //51 
    8.06581751709438785716606368564e67L     ,       //52 
    4.27488328406002556429801375339e69L     ,       //53 
    2.30843697339241380472092742683e71L     ,       //54 
    1.26964033536582759259651008476e73L     ,       //55 
    7.10998587804863451854045647464e74L     ,       //56 
    4.05269195048772167556806019054e76L     ,       //57 
    2.35056133128287857182947491052e78L     ,       //58 
    1.38683118545689835737939019720e80L     ,       //59 
    8.32098711274139014427634118320e81L     ,       //60 
    5.07580213877224798800856812177e83L     ,       //61 
    3.14699732603879375256531223550e85L     ,       //62 
    1.98260831540444006411614670836e87L     ,       //63 
    1.26886932185884164103433389335e89L     ,       //64 
    8.24765059208247066672317030680e90L     ,       //65 
    5.44344939077443064003729240248e92L     ,       //66 
    3.64711109181886852882498590966e94L     ,       //67 
    2.48003554243683059960099041857e96L     ,       //68 
    1.71122452428141311372468338881e98L     ,       //69 
    1.19785716699698917960727837217e100L    ,       //70 
    8.50478588567862317521167644240e101L    ,       //71 
    6.12344583768860868615240703853e103L    ,       //72 
    4.47011546151268434089125713813e105L    ,       //73 
    3.30788544151938641225953028221e107L    ,       //74 
    2.48091408113953980919464771166e109L    ,       //75 
    1.88549470166605025498793226086e111L    ,       //76 
    1.45183092028285869634070784086e113L    ,       //77 
    1.13242811782062978314575211587e115L    ,       //78 
    8.94618213078297528685144171540e116L    ,       //79 
    7.15694570462638022948115337232e118L    ,       //80 
    5.79712602074736798587973423158e120L    ,       //81 
    4.75364333701284174842138206989e122L    ,       //82 
    3.94552396972065865118974711801e124L    ,       //83 
    3.31424013456535326699938757913e126L    ,       //84 
    2.81710411438055027694947944226e128L    ,       //85 
    2.42270953836727323817655232034e130L    ,       //86 
    2.10775729837952771721360051870e132L    ,       //87 
    1.85482642257398439114796845646e134L    ,       //88 
    1.65079551609084610812169192625e136L    ,       //89 
    1.48571596448176149730952273362e138L    ,       //90 
    1.35200152767840296255166568759e140L    ,       //91 
    1.24384140546413072554753243259e142L    ,       //92 
    1.15677250708164157475920516231e144L    ,       //93 
    1.08736615665674308027365285257e146L    ,       //94 
    1.03299784882390592625997020994e148L    ,       //95 
    9.91677934870949689209571401540e149L    ,       //96 
    9.61927596824821198533284259496e151L    ,       //97 
    9.42689044888324774562618574310e153L    ,       //98 
    9.33262154439441526816992388560e155L    ,       //99 
    9.33262154439441526816992388563e157L    ,       //100
    9.42594775983835942085162312450e159L    ,       //101
    9.61446671503512660926865558700e161L    ,       //102
    9.90290071648618040754671525458e163L    ,       //103
    1.02990167451456276238485838648e166L    ,       //104
    1.08139675824029090050410130580e168L    ,       //105
    1.14628056373470835453434738414e170L    ,       //106
    1.22652020319613793935175170104e172L    ,       //107
    1.32464181945182897449989183712e174L    ,       //108
    1.44385958320249358220488210246e176L    ,       //109
    1.58824554152274294042537031271e178L    ,       //110
    1.76295255109024466387216104711e180L    ,       //111
    1.97450685722107402353682037276e182L    ,       //112
    2.23119274865981364659660702122e184L    ,       //113
    2.54355973347218755712013200419e186L    ,       //114
    2.92509369349301569068815180482e188L    ,       //115
    3.39310868445189820119825609359e190L    ,       //116
    3.96993716080872089540195962950e192L    ,       //117
    4.68452584975429065657431236281e194L    ,       //118
    5.57458576120760588132343171174e196L    ,       //119
    6.68950291344912705758811805409e198L    ,       //120
    8.09429852527344373968162284545e200L    ,       //121
    9.87504420083360136241157987140e202L    ,       //122
    1.21463043670253296757662432419e205L    ,       //123
    1.50614174151114087979501416199e207L    ,       //124
    1.88267717688892609974376770249e209L    ,       //125
    2.37217324288004688567714730514e211L    ,       //126
    3.01266001845765954480997707753e213L    ,       //127
    3.85620482362580421735677065923e215L    ,       //128
    4.97450422247728744039023415041e217L    ,       //129
    6.46685548922047367250730439554e219L    ,       //130
    8.47158069087882051098456875820e221L    ,       //131
    1.11824865119600430744996307608e224L    ,       //132
    1.48727070609068572890845089118e226L    ,       //133
    1.99294274616151887673732419418e228L    ,       //134
    2.69047270731805048359538766215e230L    ,       //135
    3.65904288195254865768972722052e232L    ,       //136
    5.01288874827499166103492629211e234L    ,       //137
    6.91778647261948849222819828311e236L    ,       //138
    9.61572319694108900419719561353e238L    ,       //139
    1.34620124757175246058760738589e241L    ,       //140
    1.89814375907617096942852641411e243L    ,       //141
    2.69536413788816277658850750804e245L    ,       //142
    3.85437071718007277052156573649e247L    ,       //143
    5.55029383273930478955105466055e249L    ,       //144
    8.04792605747199194484902925780e251L    ,       //145
    1.17499720439091082394795827164e254L    ,       //146
    1.72724589045463891120349865931e256L    ,       //147
    2.55632391787286558858117801578e258L    ,       //148
    3.80892263763056972698595524351e260L    ,       //149
    5.71338395644585459047893286526e262L    ,       //150
    8.62720977423324043162318862650e264L    ,       //151
    1.31133588568345254560672467123e267L    ,       //152
    2.00634390509568239477828874699e269L    ,       //153
    3.08976961384735088795856467036e271L    ,       //154
    4.78914290146339387633577523906e273L    ,       //155
    7.47106292628289444708380937294e275L    ,       //156
    1.17295687942641442819215807155e278L    ,       //157
    1.85327186949373479654360975305e280L    ,       //158
    2.94670227249503832650433950735e282L    ,       //159
    4.71472363599206132240694321176e284L    ,       //160
    7.59070505394721872907517857094e286L    ,       //161
    1.22969421873944943411017892849e289L    ,       //162
    2.00440157654530257759959165344e291L    ,       //163
    3.28721858553429622726333031164e293L    ,       //164
    5.42391066613158877498449501421e295L    ,       //165
    9.00369170577843736647426172359e297L    ,       //166
    1.50361651486499904020120170784e300L    ,       //167
    2.52607574497319838753801886917e302L    ,       //168
    4.26906800900470527493925188890e304L    ,       //169
    7.25741561530799896739672821113e306L    ,       //170
};

template<class Real>
Real GammaFunc_<Real>::factorial(Real n)
{
    //Use lookup table if integer and within table range
    if (Alge::frac(n) == 0 && n < factorialTableSize)
        return factorialTable[(int)n];

    return Alge::exp(factorialLn(n));
}

template<class Real>
Real GammaFunc_<Real>::factorialLn(Real n)
{
    return gammaLn(n+1);
}

template<class Real>
Real GammaFunc_<Real>::chooseLn(Real n, Real m)
{
    if (n < 0 || m < 0 || m > n)
        return Real_::nan;
    if (m == 0 || m == n)
        return 0;
    return factorialLn(n) - factorialLn(m) - factorialLn(n-m);
}

template class GammaFunc_<Float>;
template class GammaFunc_<Double>;


//==============================================================================================================
// Class to evaluate incomplete gamma function
//==============================================================================================================

template<class Real>
class GammaInc
{
    typedef typename Numeral<Real>::Real_ Real_;
    typedef Alge_<Real>         Alge;
    typedef GammaFunc_<Real>    GammaFunc;
    typedef Gaussian_<Real>     Gaussian;

public:
    static Real calc(Real a, Real x);
    static Real calcComp(Real a, Real x);
    static Real calcCompInv(Real a, Real y0);
private:
    static const Real big;
    static const Real bigInv;
};

template<class Real> const Real GammaInc<Real>::big = 4.503599627370496e15;
template<class Real> const Real GammaInc<Real>::bigInv =  2.22044604925031308085e-16;

/*
  * Complemented incomplete gamma integral
  *
  * igamc(a,x)   =   1 - igam(a,x)
  *
  *                           inf.
  *                             -
  *                    1       | |  -t  a-1
  *              =   -----     |   e   t   dt.
  *                   -      | |
  *                  | (a)    -
  *                            x
  *
  * In this implementation both arguments must be positive.
  * The integral is evaluated by either a power series or
  * continued fraction expansion, depending on the relative
  * values of a and x.
  */

template<class Real>
Real GammaInc<Real>::calcComp(Real a, Real x)
{
    if (x <= 0 || a <= 0)
        return 1;

    if (x < 1 || x < a)
        return 1 - calc(a,x);

    Real ax = a * Alge::log(x) - x - GammaFunc::gammaLn(a);
    if (ax < -Alge::logMax)
    {
        return 0;
    }
    ax = Alge::exp(ax);

    // continued fraction
    Real y = 1 - a;
    Real z = x + y + 1;
    Real c = 0;
    Real pkm2 = 1;
    Real qkm2 = x;
    Real pkm1 = x + 1;
    Real qkm1 = z * x;
    Real ans = pkm1/qkm1;
    Real t;
    do
    {
        c += 1;
        y += 1;
        z += 2;
        Real yc = y * c;
        Real pk = pkm1 * z  -  pkm2 * yc;
        Real qk = qkm1 * z  -  qkm2 * yc;
        if (qk != 0)
        {
            Real r = pk/qk;
            t = Alge::abs((ans - r)/r);
            ans = r;
        }
        else
            t = 1;

        pkm2 = pkm1;
        pkm1 = pk;
        qkm2 = qkm1;
        qkm1 = qk;

        if(Alge::abs(pk) > big)
        {
            pkm2 *= bigInv;
            pkm1 *= bigInv;
            qkm2 *= bigInv;
            qkm1 *= bigInv;
        }

    } while(t > Real_::epsilon);

    return ans * ax;
}


/*
  * Incomplete gamma integral
  *
  *                          x
  *                           -
  *                  1       | |  -t  a-1
  * igam(a,x)  =   -----     |   e   t   dt.
  *                 -      | |
  *                | (a)    -
  *                          0
  *
  *
  * In this implementation both arguments must be positive.
  * The integral is evaluated by either a power series or
  * continued fraction expansion, depending on the relative
  * values of a and x.
  */
/* left tail of incomplete gamma function:
  *
  *         inf.      k
  *  a  -x   -       x
  * x  e     >   ----------
  *          -     -
  *         k=0   | (a+k+1)
  *
  */

template<class Real>
Real GammaInc<Real>::calc(Real a, Real x)
{
    Real ans, ax, c, r;

    if (x <= 0 || a <= 0)
        return 0;

    if (x > 1 && x > a)
        return 1 - calcComp(a,x);

    // Compute  x**a * Alge::exp(-x) / Gamma(a)
    ax = a * Alge::log(x) - x - GammaFunc::gammaLn(a);
    if (ax < -Alge::logMax)
        return 0;
    ax = Alge::exp(ax);

    // power series
    r = a;
    c = 1;
    ans = 1;

    do
    {
        r += 1;
        c *= x/r;
        ans += c;
    } while(c/ans > Real_::epsilon);

    return ans * ax/a;
}


/// Inverse of complemented imcomplete gamma integral
/*
  * Given p, the function finds x such that
  * igamc( a, x ) = p
  *
  * Starting with the approximate value
  *        3
  * x = a t
  *
  * where
  *
  * t = 1 - d - ndtri(p) sqrt(d)
  *
  * and
  *
  * d = 1/9a,
  *
  * the routine performs up to 10 Newton iterations to find the
  * root of igamc(a,x) - p = 0.
  */
template<class Real>
Real GammaInc<Real>::calcCompInv(Real a, Real y0)
{
    if (y0 >= 1)
        return 0;
    else if (y0 <= 0)
        return Real_::inf;

    /* bound the solution */
    Real x0 = Real_::max;
    Real yl = 0;
    Real x1 = 0;
    Real yh = 1;
    Real dithresh = 5 * Real_::epsilon;

    /* approximation to inverse function */
    Real d = 1/(9*a);
    Real y = 1 - d - Gaussian().cdfInv(y0) * Alge::sqrt(d);
    Real x = a * y * y * y;

    Real lgm = GammaFunc::gammaLn(a);

    for(int i = 0; i < 10; ++i)
    {
        if (x > x0 || x < x1)
            goto ihalve;
        y = calcComp(a,x);
        if (y < yl || y > yh)
            goto ihalve;
        if (y < y0)
        {
            x0 = x;
            yl = y;
        }
        else
        {
            x1 = x;
            yh = y;
        }
        /* compute the derivative of the function at this point */
        d = (a - 1) * Alge::log(x) - x - lgm;
        if (d < -Alge::logMax)
            goto ihalve;
        d = -Alge::exp(d);
        /* compute the step to the next approximation of x */
        d = (y - y0)/d;
        if (Alge::abs(d/x) < Real_::epsilon )
            return x;
        x = x - d;
    }

    /* Resort to interval halving if Newton iteration did not converge. */
    ihalve:

    d = 0.0625;
    if( x0 == Real_::max )
    {
        if( x <= 0 )
            x = 1;
        while (x0 == Real_::max)
        {
            x = (1 + d) * x;
            y = calcComp( a, x );
            if( y < y0 )
            {
                x0 = x;
                yl = y;
                break;
            }
            d = d + d;
        }
    }
    d = 0.5;
    int dir = 0;

    for(int i = 0; i < 400; ++i)
    {
        x = x1 + d * (x0 - x1);
        y = calcComp(a, x);
        lgm = (x0 - x1)/(x1 + x0);
        if (Alge::abs(lgm) < dithresh)
            break;
        lgm = (y - y0)/y0;
        if (Alge::abs(lgm) < dithresh)
            break;
        if (x <= 0)
            break;
        if (y >= y0)
        {
            x1 = x;
            yh = y;
            if (dir < 0)
            {
                dir = 0;
                d = 0.5;
            }
            else if (dir > 1)
                d = 0.5 * d + 0.5; 
            else
                d = (y0 - yl)/(yh - yl);
            dir += 1;
        }
        else
        {
            x0 = x;
            yl = y;
            if (dir > 0)
            {
                dir = 0;
                d = 0.5;
            }
            else if (dir < -1)
                d = 0.5 * d;
            else
                d = (y0 - yl)/(yh - yl);
            dir -= 1;
        }
    }

    return x;
}

//==============================================================================================================

template<class Real>
Real Gamma_<Real>::cdf(Real x) const
{
    return GammaInc::calc(Double(a), Double(x)/Double(b));
}

template<class Real>
Real Gamma_<Real>::cdfComp(Real x) const
{
    return GammaInc::calcComp(Double(a), Double(x)/Double(b));
}

template<class Real>
Real Gamma_<Real>::cdfInv(Real P) const
{
    return Double(b)*GammaInc::calcCompInv(Double(a), Double(1-P));
}

template class Gamma_<Float>;
template class Gamma_<Double>;

}
