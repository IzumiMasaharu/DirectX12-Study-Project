#include "DXApp.h"
#include "UploadBuffer.h"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "RenderItem.h"

class MyApp : public DXApp
{
public:
	explicit MyApp(HINSTANCE hInstace);
	~MyApp() = default;
public:
	LRESULT MessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) override;
	bool Init() override;
private:
	void Resize() override;
	void Update(const GameTimer& GTimer) override;
	void Draw(const GameTimer& GTimer) override;
	void MouseDown(WPARAM ButtonState, int x, int y) override;
	void MouseUp(WPARAM ButtonState, int x, int y) override;
	void MouseMove(WPARAM ButtonState, int x, int y) override;
	void MouseWheel(short zDelta) override;
	
	void BuildRootSignature();//������ǩ��
	void BuildShaders();//��ɫ����������
	void BuildInputLayout();//�������벼��
	void BuildMeshGeometry();//����������
	void BuildMaterials();//��������
	void BuildRenderItems();//������Ⱦ��
	void BuildFrameResources();//����֡��Դ
	void BuildDescriptorHeaps();//������������������������ѣ�����ʼ��ʱ������DSV��RTV�������ѣ�
	void BuildConstantBufferViews();//��������������
	void BuildPSOs();//������Ⱦ����״̬����

	void ChangeW_H(int width, int height);
	void ChangePSOstate();
	void UpdateCamara();//��������ͷ����
	void UpdateObjectsConstBuffers()const;//���³������������������
	void UpdatePassConstBuffers()const;//������Ⱦ���̳���������
	void UpdateMaterialConstBuffers()const;//���²��ʳ���������

	void DrawRenderItems(ID3D12GraphicsCommandList* commandList, const std::vector<RenderItem*>& renderItems)const;//������Ⱦ��
public:
	const MyApp* GetMyApp()const;//��ȡָ��MyApp�������ָ��
private:
	WindowClass WC1;
	Window AppMainWin;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> mRootSignature = nullptr;//��ǩ��
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mCbvDescriptorHeap = nullptr;//CBV��������
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;//SRV��������

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;//���벼��
	UINT mPassCbvOffset = 0;//��Ⱦ���̳���������ƫ����

	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> mShaders;//������ɫ��������ͼ
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeos;//���漸�������������ͼ
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3D12PipelineState>> mPSOs;//���治ͬPSO������ͼ
	std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;//�洢���ʵ�����ͼ
	
	std::vector<std::unique_ptr<RenderItem>> mAllRenderItems;//������������Ⱦ��
	std::vector<RenderItem*> mOpaqueRenderItems;//
	std::vector<RenderItem*> mTransparentRenderItems;//

	UINT mCurrentFrameResourceIndex = 0;//��ǰ֡��Դ����
	std::vector<std::unique_ptr<FrameResource>> mFrameResources;//ȫ��֡��Դ
	FrameResource* mCurrentFrameResource = nullptr;//��ǰ֡��Դ

	POINT mLastMousePos;

	bool mIsWireframe = false;

	FLOAT RGBA[4] = { 0.0f,0.0f,0.0f,1.0f };

	DirectX::XMFLOAT3 mEyePos = { 0.0f,0.0f,0.0f };
	float mTheta = 0;//����-ԭ����x-z����ͶӰ��x��������н�
	float mPhi = DirectX::XM_PIDIV4;//����-ԭ��������Y��������н�
	float mRadius = 15.0f;//������

	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();
};

