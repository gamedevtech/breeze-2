#define BE_RENDERABLE_INCLUDE_WORLD
#define BE_RENDERABLE_INCLUDE_PROJ

#include <Engine/Perspective.fx>
#include <Engine/Renderable.fx>
#include <Pipelines/LPR/Geometry.fx>

cbuffer SetupConstants
{
	float4 DiffuseColor
	<
		String UIName = "Diffuse";
		String UIWidget = "Color";
	> = float4(1.0f, 1.0f, 1.0f, 0.03f);

	float4 SpecularColor
	<
		String UIName = "Specular";
		String UIWidget = "Color";
	> = float4(1.0f, 1.0f, 1.0f, 0.0f);
}

struct Vertex
{
	float4 Position	: Position;
	float3 Normal	: Normal;
};

struct Pixel
{
	float4 Position		: SV_Position;
	float4 NormalDepth	: TexCoord0;
};

Pixel VSMain(Vertex v)
{
	Pixel o;
	
	o.Position = mul(v.Position, WorldViewProj);
	o.Position.z = 0.0f;
	o.NormalDepth.xyz = normalize( mul((float3x3) WorldInverse, v.Normal) );
	o.NormalDepth.w = o.Position.w;
	
	return o;
}

GBufferBinding PSGeometry(Pixel p)
{
	return BindGBuffer(
		MakeGeometry(p.NormalDepth.w, normalize(p.NormalDepth.xyz)),
		MakeDiffuse(DiffuseColor.xyz, 1.0f - SpecularColor.w),
		MakeSpecular(SpecularColor.xyz, 1.0f, DiffuseColor.w) );
}

technique11 Geometry <
	string PipelineStage = "GeometryPipelineStage";
	string RenderQueue = "DefaultRenderQueue";
>
{
	pass
	{
		SetVertexShader( CompileShader(vs_4_0, VSMain()) );
		SetGeometryShader( NULL );
		SetPixelShader( CompileShader(ps_4_0, PSGeometry()) );
	}
}

technique11 <
	string IncludeEffect = "Prototypes/Feedback.fx";
> { }
