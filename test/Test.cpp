// Honeycomb, Copyright (C) 2015 NewGamePlus Inc.  Distributed under the Boost Software License v1.0.

#include "Test.h"
#include "Honey/Honeycomb.h"
#include "MtMap.h"

namespace honey
{

void test()
{
    //=============================
    //LockFree list test
    //=============================
    {
        typedef lockfree::List<int> List;

        struct ListThread
        {
            static void run(List& list)
            {
                Chacha rand;
                int data;
                int count = 100;
                for (int i = 0; i < count; ++i)
                {
                    switch (Discrete(rand, 0, 5).nextInt())
                    {
                    case 0:
                        list.pushFront(i);
                        break;
                    case 1:
                        list.pushBack(i);
                        break;
                    case 2:
                        list.popFront(data);
                        break;
                    case 3:
                        list.popBack(data);
                        break;
                    case 4:
                        {
                            auto it = list.begin();
                            for (int j = 0, end = Discrete(rand, 0, list.size()).nextInt(); j < end; ++j) ++it;
                            list.insert(it, i);
                            break;
                        }
                    case 5:
                        {
                            auto it = list.begin();
                            for (int j = 0, end = Discrete(rand, 0, list.size()).nextInt(); j < end; ++j) ++it;
                            if (it != list.end()) list.erase(it, data);
                            break;
                        }
                    }
                }
            }            
        };
        
        List list;
        vector<Thread> threads;
        for (auto i : range(7)) { threads.push_back(Thread([&]{ ListThread::run(list); })); mt_unused(i); }
        for (auto& e : threads) e.start();
        for (auto& e : threads) e.join();

        int count = 0; mt_unused(count);
        debug_print(sout() << "List Size: " << list.size() << endl);
        for (auto& e : list) { debug_print(sout() << "List " << count++ << " : " << e << endl); mt_unused(e); }
    }
    //=============================

    //=============================
    //Concurrent deque test
    //=============================
    {
        typedef concur::Deque<int> List;

        struct ListThread
        {
            static void run(List& list)
            {
                Chacha rand;
                int data;
                int count = 100;
                for (int i = 0; i < count; ++i)
                {
                    switch (Discrete(rand, 0, 3).nextInt())
                    {
                    case 0:
                        list.pushFront(i);
                        break;
                    case 1:
                        list.pushBack(i);
                        break;
                    case 2:
                        list.popFront(data);
                        break;
                    case 3:
                        list.popBack(data);
                        break;
                    }
                }
            }
        };
        
        List list;
        vector<Thread> threads;
        for (auto i : range(10)) { threads.push_back(Thread([&]{ ListThread::run(list); })); mt_unused(i); }
        for (auto& e : threads) e.start();
        for (auto& e : threads) e.join();

        int data;
        int count = 0;
        debug_print(sout() << "Deque Size: " << list.size() << endl);
        while (list.popFront(data))
        {
            debug_print(sout() << "Deque " << count << " : " << data << endl);
            ++count;
        }          
    }
    //=============================
    
    task::priv::test();

    {        
        Promise<int> promise;
        Future<int> future = promise.future();
        verify(future.wait(Millisec(1)) == future::Status::timeout);
        promise.setValue(1);
        
        Promise<int> promise2;
        promise2.setValue(2);
        Future<int> future2 = promise2.future();

        future::waitAll(future, future2);
        verify(future::waitAny(future, future2) == 0);
        
        vector<Future<int>> futures;
        futures.push_back(move(future));
        futures.push_back(move(future2));
        future::waitAll(futures);
        verify(future::waitAny(futures) == futures.begin());
        
        future::whenAll().get();
        future::whenAll(FutureCreate(), FutureCreate()).get();
        verify(future::whenAll(FutureCreate(1), FutureCreate(2)).get() == make_tuple(1, 2));
        verify(future::whenAny(FutureCreate(), FutureCreate()).get() == 0);
        verify(future::whenAny(FutureCreate(1), FutureCreate(2)).get() == make_tuple(0, 1));
        
        vector<Future<int>> futures2;
        futures2.push_back(FutureCreate(1));
        futures2.push_back(FutureCreate(2));
        verify((future::whenAll(futures2).get() == vector<int>{1, 2}));
        
        Promise<int> promise3;
        vector<Future<int>> futures3;
        futures3.push_back(promise3.future());
        futures3.push_back(FutureCreate(2));
        verify(future::whenAny(futures3).get() == make_tuple(futures3.begin()+1, 2));
        
        Promise<void> promise4;
        vector<Future<void>> futures4;
        futures4.push_back(promise4.future());
        futures4.push_back(FutureCreate());
        verify(future::whenAny(futures4).get() == futures4.begin()+1);
        
        SharedFuture<int> shared = SharedFutureCreate(1);
        SharedFuture<int> shared2 = shared;
        verify(shared.get() == 1);
        verify(shared2.get() == 1);

        PackagedTask<void (int)> task = [](int a) { !a ? throw_ Exception() << "test0" : throw std::runtime_error("test1"); };
        for (auto i : range(2))
        {
            task(i);
            try { task.future().get(); } catch (Exception& e) { debug_print(e.what_()); }
            task.reset();
        }

        int a = 1;
        PackagedTask<int& (int&)> task2([](int& a) -> int& { return a; });
        task2(a);
        verify((++task2.future().get(), a) == 2);
        
        auto outer = future::async([](int x) { return future::async([=] { return x+1; }); }, 1);
        auto inner = outer.unwrap();
        verify(inner.get() == 2);
        auto outer_s = future::async([] { return future::async([] { return 3; }).share(); });
        auto inner_s = outer_s.unwrap();
        verify(inner_s.get() == 3);
        auto s_outer = outer.share();
        auto s_inner = s_outer.unwrap();
        verify(s_inner.get() == 2);
        
        auto cont = future::async([] { return 1; }).then([](Future<int> f) { return String(sout() << f.get()+1); }).then(
            [](Future<String> f) { return atoi(f.get().u8().c_str()); });
        verify(cont.get() == 2);
    }

    mtmap_test();
    
    {
        istringstream is(R"json(
        /*
         * A sample configuration file
         */
        {
            // Comment
            "BuildType": "debug",
            "DistServer": "http://www.test.com",
            "SecondaryDistServers": [
              "http://first.fictional.server",
              "http://second.fictional.server"
            ],
            
            "Logging":
            {
                "level": "BP_LOG_LEVEL",
                "dest": "BP_LOG_DEST",
                "layout": "standard",
                "timeFormat": "utc",
                "fileRolloverKB": 2048,
                "serviceLogMode": "combined",
                "Filters":
                {
                   "enabled": true,
                   "url": false,
                   "id": true
                }
            },
            
            // Sample comments
            // more comments
            "Options":"",
            "MaxIdleSecs": 5,
            "UsageReporting":
            {
               "enabled": true,
               "url": false,
               "id": true
            },
            "ServiceUpdatePollPeriod": 86400,
            "Something" : null,
            "Rank2Array" : [[1, 2, "blah"], [3, 4, "bleh"]]
        }
        
        garbage
        
        )json");
        
        json::Value_<json::Config<true>> val;
        is >> val;
        debug_print(sout() << json::beautify << val);
    }
    
    {
        debug_print(sout() << "Tuple to string: " << tuple<Id,int,String>{"a"_id,2,"c"} << endl);
        debug_print(sout() << "Vector to string: " << vector<Id>{"a"_id,"b"_id,"c"_id} << endl);
        debug_print(sout() << "Set to string: " << set<Id>{"a"_id,"b"_id,"c"_id} << endl);
        debug_print(sout() << "Map to string: " << std::map<Id,int>{{"a"_id,1},{"b"_id,2},{"c"_id,3}} << endl);
    }
    
    {
        ByteBuf buf;
        ByteStream bs(&buf);
        bs << make_tuple(true, 97_b, 'b', -0xFFF, -0xFFFFFFFFFL, 0xFFFFFFFFFUL, 1.1f, 2.2);
        debug_print(sout() << "Tuple to bytes: " << buf.bytes() << endl);
        tuple<bool, byte, char, int, int64, uint64, float, double> t;
        bs >> t;
        debug_print(sout() << "Tuple from bytes: " << t << endl);
    }
    
    {
        int a = 0;
        for (auto i : range(0, 5, 2)) {  ++a; mt_unused(i); }
        for (auto i : range(0.0, 4.0, 1.3)) { ++a; mt_unused(i); }
        for (auto i : range(2)) { ++a; mt_unused(i); }
        for (auto i : range(4, 0, -2)) { ++a; mt_unused(i); }
        for (auto i : range(4.0, 0.0, -1.3)) { ++a; mt_unused(i); }
    }

    {
        int i = 0;
        {
            auto _ = ScopeGuard([&] { i = 1; });
            auto __ = ScopeGuard([&] { i = 2; });
            _.release();
            i = 3;
        }
    }

    {
        int count = 1000;
        BloomFilter<int> bloom(count, 0.01);
        for (auto i : range(count)) { bloom.insert(i); }
        
        int error = 0;
        for (auto i : range(count)) { if (!bloom.contains(i)) ++error; }
        // error == 0
        for (auto i : range(count)) { if (bloom.contains(count+i)) ++error; }
        // error ~= count * 0.01
    }

    {
        Real sinDif = -Real_::inf;
        int size = 1000;
        for (auto i : range(size))
        {
            Real angle = -Real_::piTwo*2 + i*Real_::piTwo*4 / (size-1);
            Real sin = Trig::atan2(Trig::cos(angle)*2, Trig::sin(angle)*2);
            Trig::enableSinTable(true);
            Real sinTab = Trig::atan2(Trig::cos(angle)*2, Trig::sin(angle)*2);
            Trig::enableSinTable(false);
            sinDif = Alge::max(sinDif, Alge::abs(sinTab - sin));
        }

        debug_print(sout() << "Sin Table Dif: " << sinDif << endl);
    }

    {
        vector<Vec3> vecs;
        vecs.push_back(Vec3(0,1,2));
        vecs.push_back(Vec3(3,4,5));
        vecs.push_back(Vec3(6,7,8));
        Vec3 blended = Interp::blend(vecs, Vec3(1,4,2)); mt_unused(blended);
        
        VecN v(20);
        v[10] = 1;
        v.resize(4);
        v(2) = 1;
        v = Vec3(1, 2, 3);
        v = v;
        v = VecN(3).fromZero();
        Vec3 v3 = v;
        v3.normalize();
        VecRowN vr(3);
        vr[2] = 0;
        vr(0,1) = 1;
        vr = Vec1(1);

        Double arr[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
        auto mata = Matrix4().fromArray(arr);
        mata = Matrix4().fromArray(arr, false);
        mata.toArray(arr);
        mata.toArray(arr, false);

        auto v41 = Matrix<4, 1>(2) * 2; mt_unused(v41);
        
        Real sum = Matrix4().fromScalar(1).elemAdd(1).sum(); mt_unused(sum);
        Vec2 swiz2(1,2);
        Vec3 swiz3(1,2,3);
        Vec4 swiz4(1,2,3,4);
        swiz2.yx() += 1;
        map(swiz4, swiz4.wzxy(), [](Real e) { return e+1; });
        swiz4.wzyx() = 2 * swiz4.xyzw();
        swiz3.yzx() = swiz3.xzz() + swiz3.xyz();
        swiz3 = swiz4.elemMul(swiz2.xxxy()).xzz();

        auto mat3 = Vec3(1,2,3) * VecRow3(10,100,1000); mt_unused(mat3);
        auto mat8 = (Matrix<8,8>().fromIdentity().block<4,4>(3,2).block<2,2>(1,2) = (Matrix<2,2>() << 2,3,4,5).eval()).parent().parent();
        mt_unused(mat8);
        debug_print(sout()  << "Mat3: " << endl << mat3 << endl
                            << "Mat8: " << endl << mat8 << endl
                            << "Sum8, Min8: " << mat8.sum() << ", " << mat8.min() << endl);
    }

    {
        //Test data from wikipedia "QR decomposition"
        //Q(0,:) = (-0.86,  0.39, -0.33)
        //R(0,:) = (-14, -21, 14)
        auto A = (Matrix<3,3>() <<
            12, -51, 4,
            6, 167, -68,
            -4, 24, -41).eval();
        Qrd<Real> qrd(A);
        debug_print(sout()  << "Q: " << endl << qrd.q() << endl
                            << "R: " << endl << qrd.r() << endl
                            << "A = Q*R: " << endl << qrd.q()*qrd.r() << endl);
    }

    {
        //Test data from wikipedia "Jacobi eigenvalue algorithm"
        //Eigen Val: (2585.25, 37.1015, 1.47805, 0.166633)
        auto A = (Matrix<4,4>() <<
            4,  -30,    60,     -35,
            -30, 300,   -675,   420,
            60, -675,   1620,   -1050,
            -35, 420,   -1050,  700).eval();
        MatrixN inv;
        auto eigen = Eigen<Real>(A);
        eigen.inverse(inv);
        debug_print(sout()  << "Eigen Val: " << eigen.w() << endl
                            << "Eigen Vec: " << endl << eigen.v() << endl
                            << "I = A*EigenInv(A): " << endl << A*inv << endl);
    }

    {
        //Linear LeastSqr b: (3.02448, 1.12965) ; cond: 14.994
        auto X = (Matrix<5,2>() <<
            1, 0,
            1, 3,
            1, 7,
            1, 10,
            1, 16).eval();
        auto y = (Vec<5>() << 3, 5, 10, 17, 20).eval();
        auto w = (Vec<5>() << 2, 3, 1, 4, 5).eval();
        VecN b;
        LinearLeastSqr<Real>().calc(X, y, w, b);
        debug_print(sout() << "Linear LeastSqr b: " << b << " ; cond: " << X.cond() << endl);

        Svd<Real> svd(X, Svd<Real>::Mode::full);
        MatrixN inv;
        svd.inverse(inv);
        debug_print(sout() << "X*FullSvdInv(X)*X: " << endl << X*inv*X << endl);
    }

    {
        //Test data from "Numerical Methods for Least Squares Problems"
        //b: (5.75, -0.25, 1.5) ; cond: 2
        auto X = (Matrix<4,3>() <<
            1, 1, 1,
            1, 3, 1,
            1, -1, 1,
            1, 1, 1).eval();
        auto y = (Vec<4>() << 1, 2, 3, 4).eval();
        auto w = (Vec<4>() << 1, 1, 1, 1).eval();
        auto C = (Matrix<2,3>() <<
            1, 1, 1,
            1, 1, -1).eval();
        auto d = (Vec<2>() << 7, 4).eval();
        VecN b;
        LinearLeastSqr<Real>().calc(X, y, w, C, d, b);
        debug_print(sout() << "Constrained LeastSqr b: " << b << " ; cond: " << X.cond() << endl);        
    }

    {
        //BisectN: true; (3.14159, 1.5708, 1.5708)
        typedef BisectN<Real,3> BisectN;
        BisectN::Funcs funcs =
        {
            [](const Vec3& v) { return v.x - v.y - Real_::piHalf; },
            [](const Vec3& v) { return v.x - v.y - v.z; },
            [](const Vec3& v) { return v.z - v.x + Real_::piHalf; },
        };
        auto res = BisectN().root(funcs, Vec3(-10), Vec3(10)); mt_unused(res);
        debug_print(sout() << "BisectN: " << std::boolalpha << get<0>(res) << "; " << get<1>(res) << endl);
    }

    {
        //Mean CI 95%:    (-9.19019, -2.52411)
        //Std Dev CI 95%: (2.05223, 4.72423)
        //Pr > |t|:       0.00240068
        auto vf = (Vec<7>() << 75, 76, 80, 77, 80, 77, 73).eval();
        auto vm = (Vec<7>() << 82, 80, 85, 85, 78, 87, 82).eval();
        StudentT::PooledStats stats;
        bool t_test = StudentT::test(vf, vm, stats, 0, 0.05, 0); mt_unused(t_test);
        debug_print(sout() << "T-test: " << std::boolalpha << t_test << endl << stats << endl);
    }

    //Minimize: (2.23277, -1.99996)
    debug_print(sout() << "Minimize: " <<
                    Minimize<Real>().calc([](Real x) { return 0.2f*Alge::pow(x,4) - 2*Alge::pow(x,2) + 3; }, 0, 5, 0.1) << endl);

    //MinimizeN: ((1, 2), 0)
    debug_print(sout() << "MinimizeN: " <<
                    MinimizeN<Real,2>().calc([](const Vec2& v) { return Alge::pow(v[0]-1, 2) + Alge::pow(v[1]-2, 2); }, Vec2(-10), Vec2(10), Vec2(7, -5)) << endl);

    //Poly roots 3: ((-0.60583, 0, 0), 1) Bounds: (0.2, 1.75)
    //Poly roots 4: ((5, 3, -4, -6), 4)
    debug_print(sout() << "Poly roots 1: " << Polynomial<Real>::roots(Vec2(1,2)) << endl);
    debug_print(sout() << "Poly roots 2: " << Polynomial<Real>::roots(Vec3(1,2,3)) << endl);
    debug_print(sout() << "Poly roots 3: " << Polynomial<Real>::roots(Vec4(1,2,3,4)) << " Bounds: " << Polynomial<Real>::rootBounds(Vec4(1,2,3,4)) << endl);
    debug_print(sout() << "Poly roots 4: " << Polynomial<Real>::roots((Vec<5>() << 1080,-126,-123,6,3).eval()) << endl);
    debug_print(sout() << "Poly roots 4 (generic): " << Polynomial<Real>::roots((VecN().resize(5) << 1080,-126,-123,6,3).eval(), 1e-04f) << endl);

    assert(String::join(String("foo bar blah").split()) == "foo bar blah");
    assert(String::join(String::List{"foo", "bar", "blah"}) == "foo bar blah");

    {
        Chacha gen;
        Double x;
        Vec1 dx1; Vec2 dx2; Vec3 dx3; Vec4 dx4;
        x = SimplexNoise<1,Float>(gen).noise(Vec1(0.5), dx1);
        x = SimplexNoise<1,Double>(gen).noise(Vec1(0.5));
        x = SimplexNoise<2,Float>(gen).noise(Vec2(0.5), dx2);
        x = SimplexNoise<2,Double>(gen).noise(Vec2(0.5));
        x = SimplexNoise<3,Float>(gen).noise(Vec3(0.5), dx3);
        x = SimplexNoise<3,Double>(gen).noise(Vec3(0.5));
        x = SimplexNoise<4,Float>(gen).noise(Vec4(0.5), dx4);
        x = SimplexNoise<4,Double>(gen).noise(Vec4(0.5));
    }
    
    Real f = 0.5, f2;
    Vec3 v1, v2, v3;
    v1 = Vec3(3.f, 4.5f, 1.2f);
    v1 = v1.normalize();

    v2 = Vec3(1.f, 2.5f, 3.2f);
    v2 = v2.normalize();

    v3 = Vec3(4.f, 0.5f, 2.2f);

    Quat q0, q1, q2, q3, q4;
    q0.fromAlign(v1, v2);
    q1.fromAlign(v2, v1);
    q2.fromEulerAngles(Vec3(4.f, 1.5f, 5.2f));
    q3.fromAxisAngle(Vec3::axisX, Real_::piHalf);
    
    Quat a, b, c;
    Quat::squadSetup(q0, q1, q2, q3, a, b, c);
    q4 = q2;
    q4 = q4.inverse();

    Transform tm, tm2, tm3, tm4;
    tm2.setTrans(v1);
    tm2.setScale(Vec3(1,2,2));
    tm3.setRot(q3);
    tm4.setScale(2);
    tm = tm2*tm3;
    tm = tm.inverse();
    tm = tm*tm4;

    Matrix4 mat, mat2, mat3, mat4;
    mat2.fromIdentity().setTrans(v1);
    mat2.setScale(Vec3(1,2,2));
    mat3.fromIdentity().setRot(q3);
    mat4.fromIdentity().setScale(2);
    mat = mat2*mat3;
    mat = mat.inverse();
    mat = mat*mat4;

    typedef Vec<6> Vec6;
    auto v6 = Vec6().fromAxis(0);
    v6 += Vec6().fromScalar(1)*5;
    v6 = v6.normalize(f);

    typedef Matrix<6,6> Matrix6;
    auto mat6 = Matrix6().fromIdentity();
    mat6 += Matrix6(1)*5;
    mat6 = mat6.inverse(f);
    mat6.transposeInPlace();

    debug_print(sout()
        << "Vec1:   "   << v1 << endl
        << "Vec2:   "   << v2 << endl
        << "Vec3:   "   << v3 << endl
        << "Vec6:   "   << v6 << endl
        << "Real:   "   << f << endl
        << "Quat0:  "   << q0 << endl
        << "Quat3:  "   << q3 << endl
        << "Quat4:  "   << q4 << endl
        << "Tm:     "   << tm << endl
        );
    
    Chacha gen;
    f = Uniform(gen).next();
    Double fd = Uniform_d(gen).next(); mt_unused(fd);
    f = Discrete(gen, 10, 20).next();
    f = Discrete(gen).next();
    f = Discrete(gen).pdf(0);
    f = Discrete(gen).cdf(100);
    f = Discrete(gen).cdfComp(100);
    f = Discrete(gen).cdfInv(0.5);
    f = Discrete(gen).mean();
    f = Discrete(gen).variance();
    f = Discrete(gen).stdDev();

    Chacha::State state = gen.getState();
    f2 = Gaussian(gen, 0, 10).next();
    gen.setState(state);
    f2 = Gaussian(gen, 0, 10).next();
    
    DiscreteGen::List pdf;
    pdf.push_back(1);
    pdf.push_back(2);
    pdf.push_back(10);
    pdf.push_back(12);
    pdf.push_back(9);
    pdf.push_back(2);
    pdf.push_back(6);
    pdf.push_back(15);
    pdf.push_back(2);
    pdf.push_back(2);
    pdf.push_back(3);
    pdf.push_back(2);
    pdf.push_back(3);
    pdf.push_back(4);
    pdf.push_back(8);
    pdf.push_back(9);
    pdf.push_back(2);
    pdf.push_back(8);
    pdf.push_back(4);
    pdf.push_back(1);
    pdf.push_back(5);
    pdf.push_back(8);
    pdf.push_back(1);
    pdf.push_back(6);
    pdf.push_back(2);
    pdf.push_back(10);
    pdf.push_back(12);
    pdf.push_back(6);
    pdf.push_back(1);
    pdf.push_back(15);
    DiscreteGen disc(gen, pdf);
    //HyperGeo disc(gen, 200, 50, 90);
    debug_print(sout() << "Disc Mean: " <<  disc.mean() << " ; Disc Var: " << disc.variance() << endl);

    vector<Vec1> samples;
    for (auto i : range(100))
    {
        mt_unused(i);
        samples.push_back(disc.next());
    }

    typedef Bootstrap<Vec1> Bootstrap;

    Bootstrap bootMean(Bootstrap::meanFunc(), gen, samples);
    bootMean.calc();
    debug_print(sout() << "Boot Mean: " << bootMean.lower() << " ; " << bootMean.upper() << endl);

    Bootstrap bootVar(Bootstrap::varianceFunc(), gen, samples);
    bootVar.calc();
    debug_print(sout() << "Boot Var: " << bootVar.lower() << " ; " << bootVar.upper() << endl);

    for (auto i : range(-1, 10))
    {
        mt_unused(i);
        f = disc.next();
        f2 = disc.cdfInv(disc.cdf(f));
        debug_print(sout() << "Dif: " << std::setw(4) << f << " ; " << std::setw(4) << f2 << " ; " << Alge_d::abs(f-f2) << endl);
    }

    vector<Real> list;
    list.push_back(1);
    list.push_back(2);
    list.push_back(3);
    list.push_back(4);
    vector<Real> sample;
    vector<int> unchosen;
    Random(gen).choose(list, 3, sample, unchosen);
    Random(gen).shuffle(sample);

    struct PermuteFunc
    {
        bool operator()(const vector<const Real*>& perm)
        {
            if (perm.size() == 2 && *perm[0] == 1 && *perm[1] == 4)
                return false;
            if (perm.size() == 2 && *perm[0] == 2 && *perm[1] == 3)
                return false;
            if (perm.size() == 3 && *perm[0] == 2 && *perm[1] == 4 && *perm[2] == 3)
                return false;
            if (perm.size() == 3 && *perm[0] == 4 && *perm[1] == 3 && *perm[2] == 1)
                return false;
            return true;
        }
    };

    auto permute = Permute::range(list, PermuteFunc());
    for (auto it = begin(permute); it != end(permute); ++it)
    {
        debug_print("Perm: ");
        for (auto i : range((int)it->size())) { debug_print(sout() << *it->at(i) << " ");  mt_unused(i); }
        debug_print(sout() << " ; Cnt: " << it.count() << "\n");
    }

    Chacha crypt;
    Chacha::Key key;
    Chacha::Iv iv, iv2;
    for (auto i : range(0, size(key), (int)sizeof(uint32))) BitOp::toPartsBig(Discrete_<uint32>::nextStd(gen), key.data() + i);
    for (auto i : range(0, size(iv), (int)sizeof(uint32))) BitOp::toPartsBig(Discrete_<uint32>::nextStd(gen), iv.data() + i);
    for (auto i : range(0, size(iv2), (int)sizeof(uint32))) BitOp::toPartsBig(Discrete_<uint32>::nextStd(gen), iv2.data() + i);
    
    String msg = "Test msg la la la la ";
    for (auto i : range(50)) { msg += sout() << i << " "; }
        
    String msg2 = "Test2 msg2 la la la la";
    byte cipher[1000];
    char decipher[1000];

    crypt.setKey(key);
    crypt.setIv(iv);
    crypt.encrypt(reinterpret_cast<const byte*>(msg.u8().c_str()), cipher, msg.length());
    crypt.setIv(iv2);
    crypt.encrypt(reinterpret_cast<const byte*>(msg2.u8().c_str()), cipher+msg.length(), msg2.length());

    crypt.setKey(key);
    crypt.setIv(iv);
    crypt.decrypt(cipher, reinterpret_cast<byte*>(decipher), msg.length());
    crypt.setIv(iv2);
    crypt.decrypt(cipher+msg.length(), reinterpret_cast<byte*>(decipher+msg.length()), msg2.length());
    decipher[msg.length()+msg2.length()] = 0;
    
    {
        debug_print(sout() << "Hash 1: " << hash::fast("some string") << " " << fromBytes<int>(toBytes(hash::fast("some string"))) << endl);
        debug_print(sout() << "Hash 2: " << toBytes(hash::fast("some string")) << endl);
        debug_print(sout() << "Hash 3: " << toBytes(hash::fast("some string", 1)) << endl);
        debug_print(sout() << "Secure Hash 1: " << hash::secure("some string") << endl);
        debug_print(sout() << "Secure Hash 2: " << hash::secure("some string", hash::secure("key")) << endl);
    
        ByteArray<5> literals(255_b,'2'_b,256_b,'4'_b,5_b);
        auto time = MonoClock::now(); mt_unused(time);
        auto keys = hash::secureKeys("password", "some string"_b, 1 << 15, 2);
        debug_print(sout() << "Secure Key 1: " << keys[0] << endl);
        debug_print(sout() << "Secure Key 2: " << keys[1] << endl);
        debug_print(sout() << "Secure Key Time: " << Millisec(MonoClock::now()-time) / 1000. << endl);
    }
    
    {
        ostringstream os;
        os << encode::hex << "some string"_b;
        debug_print(sout() << "Encode Hex: " << os.str() << endl);
        istringstream is(os.str());
        Bytes bs;
        is >> encode::hex >> bs;
        debug_print(sout() << "Decode Hex: " << encode::u8 << bs << endl);
    }
    
    {
        ostringstream os;
        os << encode::dec << "some string"_b;
        debug_print(sout() << "Encode Dec: " << os.str() << endl);
        istringstream is(os.str());
        Bytes bs;
        is >> encode::dec >> bs;
        debug_print(sout() << "Decode Dec: " << encode::u8 << bs << endl);
    }
    
    {
        ostringstream os;
        os << encode::u8 << "some string"_b;
        debug_print(sout() << "Encode UTF-8: " << os.str() << endl);
        istringstream is(os.str());
        Bytes bs;
        is >> encode::u8 >> bs;
        debug_print(sout() << "Decode UTF-8: " << encode::u8 << bs << endl);
    }
    
    {
        ostringstream os;
        os << encode::base64 << "some string"_b;
        debug_print(sout() << "Encode Base64: " << os.str() << endl);
        istringstream is(os.str());
        Bytes bs;
        is >> encode::base64 >> bs;
        debug_print(sout() << "Decode Base64: " << encode::u8 << bs << endl);
    }
    
    int argc;
    char** argv;
    string::parseArgv("srhasnehrane hsanerhsra \"srasr\" 'srasra' \"sras's\" 12312", argc, argv);
    string::deleteArgv(argc, argv);
    
    Log log;
    log.filter("std"_id, {&log::level::debug}, true, {&log::level::info});
    log << log::level::debug << "test1 " << 101;
    log << log::level::info << "test2 " << 102;
    log << log::level::warning << "test3 " << 103;
    
    typedef Vegas<5, 4, Double> Vegas;
    struct VegasFunc
    {
        typedef Vegas::Real Real;
        typedef Alge_<Real> Alge;

        Vegas::VecRes operator()(const Vegas::Vec& x)
        {
            //Principal integral        0.999142 +/- 0.000812909
            //1st additional integral   2.71833  +/- 0.00406556
            //2nd additional integral   3.13637  +/- 0.00468428
            //3rd additional integral   0.998752 +/- 0.00115633
            Vegas::VecRes f;
            Real dummy, exponent, denominator;

            exponent = 0.0;
            denominator = 1.0;
            for (auto i : range(Vegas::dim))
            {
                dummy = 2*x[i]-1;
                dummy = 0.5*Alge::log((1+dummy)/(1-dummy));
                exponent -= dummy*dummy/2.0/0.2/0.2;
                dummy = 2*x[i]-1;
                denominator *= 1-dummy*dummy;
            }

            f[0] = Alge::exp(exponent)/denominator/Alge::pow(0.5*Real_::pi*0.2*0.2,Real(Vegas::dim)/2);
            if (Vegas::dimRes >= 2)
                f[1] = f[0] * 2.718281828 * (12.0*x[0]-5.0);
            if (Vegas::dimRes >= 3)
                f[2] = f[0] * 3.141592654 * (12.0*x[1]-5.0);
            if (Vegas::dimRes >= 4)
                f[3] = (x[2]<0.5) ? (f[0]*2.0) : (0.0);
            return f;
        }
    };

    Vegas vegas(VegasFunc(), gen, Vegas::Vec(0.), Vegas::Vec(1.), 10000);
    debug_print(sout() << "Vegas: " << vegas.integrate() << endl);

    Id id("foo_bar");
    switch (id)
    {
    case "eggs"_id:
        {
            debug_print("IdSwitch: eggs\n");
            break;
        }
    case "foo_bar"_id:
        {
            debug_print("IdSwitch: foo_bar\n");
            break;
        }
    default:
        {
            debug_print("IdSwitch: default\n");
            break;
        }
    }

    typedef DepNode<int> DepNode;
    DepNode depnode[10];
    for (auto i : range(10)) { depnode[i].setKey(Id(sout() << "Node " << i)); }
        
    depnode[0].add("Node 1"_id);
    depnode[0].add("Node 3"_id);
    depnode[1].add("Node 2"_id);
    depnode[2].add("Node 0"_id);

    depnode[3].add("Node 4"_id);
    depnode[4].add("Node 5"_id);
    depnode[5].add("Node 3"_id);

    depnode[6].setKey("Node 0"_id);
    depnode[6].add("Node 1"_id);
    depnode[6].add("Node 3"_id);
    
    DepGraph<DepNode> depgraph;
    for (auto i : range(10)) { depgraph.add(depnode[i]); }
    
    bool depends = depgraph.depends(depnode[0].getKey(), depnode[5]);
    depends = depgraph.depends(depnode[5].getKey(), depnode[0].getKey());

    depgraph.condense();
    depgraph.remove(depnode[6]);
    depgraph.condense();

    int depvertex = 0; mt_unused(depvertex);
    for (auto& e : depgraph.range(depnode[0].getKey()))
    {
        debug_print(sout() << "DepVertex " << depvertex++ << endl);
        for (auto& e : e.nodes()) { debug_print(sout() << "    " << e->getKey() << endl); mt_unused(e); }
    }
    
    TreeNode<int> cNode;
    TreeNode<int> cNode2;

    TreeClone<TreeNode<int>> clone;
    auto& cloneNode = clone.regNode(cNode);
    clone.update();
    cNode.setData(2);
    clone.unregClone(cloneNode);
    cNode.setData(4);
    cNode.addChild(cNode2);
    clone.update();
    
    {
        struct A : public SharedObj<A>, SmallAllocatorObject
        {
            A()     : SharedObj(SmallAllocator<A>()) {}
            ~A() {}
        };

        WeakPtr<A> weak;
        {
            auto ptr = make_shared<A>();
            weak = ptr;
        }
        assert(!weak.lock());
    }
    
    vector<UniqueLock<Mutex>> locks;
    locks.resize(10);
    for (auto& e : locks) { e = UniqueLock<Mutex>(*new Mutex(), lock::Op::defer); }
    lock::lock(locks[0], locks[1]);
    lock::lock(range(locks.begin()+2, locks.end()));

    SharedMutex mutex;
    SharedMutex::Scoped rlock(mutex);
    {
        TransferLock<decltype(rlock), SharedMutex::SharedScoped> wlock(rlock);
    }

    int* blah = SmallAllocator<int>().allocate(10000);
    debug_print(SmallAllocator<int>().pool().printStats());
    SmallAllocator<int>().deallocate(blah, 1);
    SmallAllocator<int>().pool().validate();
    
    {
        int count = 10000;
        int iter = 1000000;
        int dummy = 0;
        typedef bloom_filter::Key<int> Key;
        vector<Key> keys(count, Key(count, 0.01));

        BloomFilter<Key> bloom(count, 0.01);
        for (auto i : range(0, count, 4)) { keys[i].hash(i); bloom.insert(keys[i]); }
        unordered_set<Key> set;
        for (auto i : range(0, count, 4)) { set.insert(keys[i]); }

        auto time = MonoClock::now(); mt_unused(time);
        for (int i = 0; i < iter; ++i)
            if (bloom.contains(keys[i%count])) ++dummy;
        debug_print(sout() << "Bloom Time 0: " << Millisec(MonoClock::now()-time) / 1000. << endl);

        for (int i = 0; i < iter; ++i)
            if (set.count(keys[i%count])) ++dummy;
        debug_print(sout() << "Bloom Time 1: " << Millisec(MonoClock::now()-time) / 1000. << " " << dummy << endl);
    }
}

}
