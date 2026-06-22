#include "PhysX.h"

using namespace physx;

// 初期化
void PhysX::Initialize()
{
	// Foundatio作成
	foundation = PxCreateFoundation(PX_PHYSICS_VERSION, allocator, errorCallback);

	// PhysXVisualDebuggerに接続
	pvd = PxCreatePvd(*foundation);
	pvdTransport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	pvd->connect(*pvdTransport, physx::PxPvdInstrumentationFlag::eALL);

	// PxPhysics本体作成
	physics = PxCreatePhysics(PX_PHYSICS_VERSION, *foundation, PxTolerancesScale(), true, pvd);

	// シミュレーションをCPUスレッドに割り振るディスパッチャを作成
	dispatcher = PxDefaultCpuDispatcherCreate(1); // スレッド数１

	// デフォルトマテリアル(静摩擦, 動摩擦, 反発係数)作成
	defaultMaterial = physics->createMaterial(0.5f, 0.5f, 0.6f);
}
// 終了処理
void PhysX::Cleanup()
{
	// 作成した順と逆順に解放する
	if (defaultMaterial) { defaultMaterial->release(); defaultMaterial = nullptr; }
	if (dispatcher)      { dispatcher->release();      dispatcher      = nullptr; }
	if (physics)         { physics->release();         physics         = nullptr; }
	if (pvd)             { pvd->disconnect();
				           pvd->release();             pvd             = nullptr; }
	if (pvdTransport)    { pvdTransport->release();    pvdTransport    = nullptr; }
	if (foundation)      { foundation->release();      foundation      = nullptr; }
}
