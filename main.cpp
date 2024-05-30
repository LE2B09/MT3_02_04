#include <Novice.h>
#define _USE_MATH_DEFINES
#include <cmath>
#include <assert.h>
#include <imgui.h>
#include "Matrix4x4.h"
#include "Vector3.h"
#include "VectorMatrix.h"

using namespace std;

static const int kWindowWidth = 1280;
static const int kWindowHeight = 720;

//線分
struct Segment
{
	Vector3 origin;		//始点
	Vector3 diff;		//終点からの差分
};

//三角形の頂点
struct Triangle
{
	Vector3 vertices[3];	//!< 頂点
};

//Gridを表示する疑似コード
static void DrawGrid(const Matrix4x4& ViewProjectionMatrix, const Matrix4x4& ViewportMatrix)
{
	const float	kGridHalfWidth = 2.0f;										//Gridの半分の幅
	const uint32_t kSubdivision = 10;										//分割数
	const float kGridEvery = (kGridHalfWidth * 2.0f) / float(kSubdivision);	//1つ分の長さ

	//水平方向の線を描画
	for (uint32_t xIndex = 0; xIndex <= kSubdivision; xIndex++)
	{
		//X軸上の座標
		float posX = -kGridHalfWidth + kGridEvery * xIndex;

		//始点と終点
		Vector3 start = { posX, 0.0f, -kGridHalfWidth };
		Vector3 end = { posX, 0.0f, kGridHalfWidth };
		// ワールド座標系 -> スクリーン座標系まで変換をかける
		start = Transform(start, Multiply(ViewProjectionMatrix, ViewportMatrix));
		end = Transform(end, Multiply(ViewProjectionMatrix, ViewportMatrix));

		Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, 0x6F6F6FFF);
	}

	//垂直方向の線を描画
	for (uint32_t zIndex = 0; zIndex <= kSubdivision; zIndex++)
	{
		//Z軸上の座標
		float posZ = -kGridHalfWidth + kGridEvery * zIndex;

		//始点と終点
		Vector3 startZ = { -kGridHalfWidth, 0.0f, posZ };
		Vector3 endZ = { kGridHalfWidth, 0.0f, posZ };
		// ワールド座標系 -> スクリーン座標系まで変換をかける
		startZ = Transform(startZ, Multiply(ViewProjectionMatrix, ViewportMatrix));
		endZ = Transform(endZ, Multiply(ViewProjectionMatrix, ViewportMatrix));

		Novice::DrawLine((int)startZ.x, (int)startZ.y, (int)endZ.x, (int)endZ.y, 0x6F6F6FFF);
	}
}

//三角形を描画するコード
void DrawTriangle(const Triangle& triangle, const Matrix4x4& viewProjectionMatrix, const Matrix4x4& viewportMatrix, uint32_t color)
{
	Vector3 screenVertices[3];
	for (int i = 0; i < 3; ++i) 
	{
		screenVertices[i] = Transform(Transform(triangle.vertices[i], viewProjectionMatrix), viewportMatrix);
	}
	Novice::DrawTriangle((int)screenVertices[0].x, (int)screenVertices[0].y,
		(int)screenVertices[1].x, (int)screenVertices[1].y,
		(int)screenVertices[2].x, (int)screenVertices[2].y,
		color, kFillModeWireFrame);
}

//三角形と線の衝突判定
bool IsCollision(const Triangle& triangle, const Segment& segment)
{
	// 三角形の辺
	Vector3 edge1 = Subtract(triangle.vertices[1], triangle.vertices[0]);
	Vector3 edge2 = Subtract(triangle.vertices[2], triangle.vertices[0]);

	// 平面の法線ベクトルを計算
	Vector3 normal = Cross(edge1, edge2);
	normal = Normalize(normal);

	// 線分の方向ベクトル
	Vector3 dir = segment.diff;
	dir = Normalize(dir);

	// 平面と線分の始点のベクトル
	Vector3 diff = Subtract(triangle.vertices[0], segment.origin);

	// 線分が平面と平行かどうかをチェック
	float dotND = Dot(normal, dir);
	if (fabs(dotND) < 1e-6f)
	{
		return false; // 線分が平面と平行
	}

	// 線分の始点と平面の交点を計算
	float t = Dot(normal, diff) / dotND;

	if (t < 0.0f || t > Length(segment.diff))
	{
		return false; // 線分上に交点がない
	}

	Vector3 intersection = Add(segment.origin, Multiply(t, dir));

	// バリツチェックで三角形の内部に交点があるかを確認
	Vector3 c0 = Cross(Subtract(triangle.vertices[1], triangle.vertices[0]), Subtract(intersection, triangle.vertices[0]));
	Vector3 c1 = Cross(Subtract(triangle.vertices[2], triangle.vertices[1]), Subtract(intersection, triangle.vertices[1]));
	Vector3 c2 = Cross(Subtract(triangle.vertices[0], triangle.vertices[2]), Subtract(intersection, triangle.vertices[2]));

	if (Dot(c0, normal) >= 0.0f && Dot(c1, normal) >= 0.0f && Dot(c2, normal) >= 0.0f)
	{
		return true; // 衝突
	}

	return false; // 衝突なし
}


const char kWindowTitle[] = "提出用課題";

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
	// ライブラリの初期化
	Novice::Initialize(kWindowTitle, kWindowWidth, kWindowHeight);

	// キー入力結果を受け取る箱
	char keys[256] = { 0 };
	char preKeys[256] = { 0 };

	Segment segment{ {0.0f, -1.0f, 0.0f}, {1.0f, 2.0f, 2.0f} };
	Triangle triangle = { {{-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}} };

	Vector3 rotate = {};
	Vector3 translate = {};
	Vector3 cameraTranslate = { 0.0f, 1.9f, -6.49f };
	Vector3 cameraRotate = { 0.26f, 0.0f, 0.0f };

	// ウィンドウの×ボタンが押されるまでループ
	while (Novice::ProcessMessage() == 0)
	{
		// フレームの開始
		Novice::BeginFrame();

		// キー入力を受け取る
		memcpy(preKeys, keys, 256);
		Novice::GetHitKeyStateAll(keys);

		///
		/// ↓更新処理ここから
		///

		ImGui::Begin("Settings");
		ImGui::DragFloat3("Segment Origin", &segment.origin.x, 0.01f);
		ImGui::DragFloat3("Segment Diff", &segment.diff.x, 0.01f);
		ImGui::DragFloat3("Triangle Vertex 0", &triangle.vertices[0].x, 0.01f);
		ImGui::DragFloat3("Triangle Vertex 1", &triangle.vertices[1].x, 0.01f);
		ImGui::DragFloat3("Triangle Vertex 2", &triangle.vertices[2].x, 0.01f);
		ImGui::DragFloat3("rotate", &rotate.x, 0.01f);
		ImGui::DragFloat3("Camera Translate", &cameraTranslate.x, 0.01f);
		ImGui::DragFloat3("Camera Rotate", &cameraRotate.x, 0.01f);
		ImGui::End();

		//各種行列の計算
		Matrix4x4 worldMatrix = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, rotate, translate);
		Matrix4x4 cameraMatrxi = MakeAffineMatrix({ 1.0f,1.0f,1.0f }, cameraRotate, cameraTranslate);
		Matrix4x4 viewWorldMatrix = Inverse(worldMatrix);
		Matrix4x4 viewCameraMatrix = Inverse(cameraMatrxi);

		// 透視投影行列を作成
		Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(kWindowWidth) / float(kWindowHeight), 0.1f, 100.0f);

		//ビュー座標変換行列を作成
		Matrix4x4 viewProjectionMatrix = Multiply(viewWorldMatrix, Multiply(viewCameraMatrix, projectionMatrix));

		//ViewportMatrixビューポート変換行列を作成
		Matrix4x4 viewportMatrix = MakeViewportMatrix(0.0f, 0.0f, float(kWindowWidth), float(kWindowHeight), 0.0f, 1.0f);

		Vector3 start = Transform(Transform(segment.origin, viewProjectionMatrix), viewportMatrix);
		Vector3 end = Transform(Transform(Add(segment.origin, segment.diff), viewProjectionMatrix), viewportMatrix);

		///
		/// ↑更新処理ここまで
		///

		///
		/// ↓描画処理ここから
		///

		// Gridを描画
		DrawGrid(viewProjectionMatrix, viewportMatrix);

		// 衝突判定
		uint32_t color = IsCollision(triangle, segment) ? 0xFF0000FF : 0xFFFFFFFF;

		// 描画処理
		DrawTriangle(triangle, viewProjectionMatrix, viewportMatrix, 0xFFFFFFFF);
		Novice::DrawLine((int)start.x, (int)start.y, (int)end.x, (int)end.y, color);

		///
		/// ↑描画処理ここまで
		///

		// フレームの終了
		Novice::EndFrame();

		// ESCキーが押されたらループを抜ける
		if (preKeys[DIK_ESCAPE] == 0 && keys[DIK_ESCAPE] != 0)
		{
			break;
		}
	}

	// ライブラリの終了
	Novice::Finalize();
	return 0;
}
