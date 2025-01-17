/////////////////////////////////////////////////////////////////////////////////////////////
// Copyright 2017 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or imlied.
// See the License for the specific language governing permissions and
// limitations under the License.
/////////////////////////////////////////////////////////////////////////////////////////////
#include "CPUT_DX11.h"
#include "CPUTRenderStateBlockDX11.h"
#include "CPUTRenderStateMapsDX11.h"

//-----------------------------------------------------------------------------
void CPUTRenderStateBlockDX11::ReadValue( CPUTConfigEntry *pValue, const CPUTRenderStateMapEntry *pRenderStateList, void *pDest )
{
    std::string lowerCaseName = pValue->NameAsString();

    bool found = false;
    // Find it in the map.  TODO: could use a real map.  Maybe with a binary search, lexical storage, etc.
    for( CPUTRenderStateMapEntry const *pCur = pRenderStateList; pCur->name.compare(""); pCur++ )
    {
        found = 0 == _stricmp( lowerCaseName.data(), pCur->name.data() );
        if( found )
        {
            // We found it.  Now convert it from the text file's string to its internal representation

            // There must be a more-generic way to do the following.  write( void*, void*, type ).
            // Use function pointer array to ValueAsInt() and similar, so we can call them without the switch?
            // Might require they all have same signature ==> use void pointers, and cast internally?
            switch( pCur->type )
            {
            case ePARAM_TYPE_TYPELESS: ASSERT(0,""); break; // Should not get here.
            case ePARAM_TYPE_INT:     *(int*)&((char*)pDest)[pCur->offset]  = pValue->ValueAsInt();   break;
            case ePARAM_TYPE_UINT:   *(UINT*)&((char*)pDest)[pCur->offset]  = pValue->ValueAsUint();  break;
            case ePARAM_TYPE_BOOL:   *(bool*)&((char*)pDest)[pCur->offset]  = pValue->ValueAsBool();  break;
            case ePARAM_TYPE_FLOAT: *(float*)&((char*)pDest)[pCur->offset]  = pValue->ValueAsFloat(); break;
            case ePARAM_TYPE_SHORT: *(short*)&((char*)pDest)[pCur->offset]  = pValue->ValueAsInt();   break;
            case ePARAM_TYPE_CHAR:            ((char*)pDest)[pCur->offset]  = pValue->ValueAsInt();   break;
            case ePARAM_TYPE_UCHAR:           ((UCHAR*)pDest)[pCur->offset] = pValue->ValueAsUint();  break;
            case ePARAM_TYPE_STRING:           strncpy(&((char*)pDest)[pCur->offset], pValue->ValueAsString().c_str(), MAX_LENGTH_RENDERSTATE_STRING); break;
                // The following types must be converted from string to enum.  They achieve this with a translation map
            case ePARAM_TYPE_D3D11_BLEND:      found = pBlendMap->FindMapEntryByName(          (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_BLEND_OP:   found = pBlendOpMap->FindMapEntryByName(        (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_DEPTH_WRITE_MASK: found = pDepthWriteMaskMap->FindMapEntryByName( (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_STENCIL_OP: found = pStencilOpMap->FindMapEntryByName(      (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_FILL_MODE:  found = pFillModeMap->FindMapEntryByName(       (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_CULL_MODE:  found = pCullModeMap->FindMapEntryByName(       (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_FILTER:     found = pFilterMap->FindMapEntryByName(         (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_COMPARISON_FUNC:      found = pComparisonMap->FindMapEntryByName(     (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            case ePARAM_TYPE_D3D11_TEXTURE_ADDRESS_MODE: found = pTextureAddressMap->FindMapEntryByName( (int*)&((char*)pDest)[pCur->offset], pValue->ValueAsString() ); break;
            }
            break; // From for.  We found it, so we're done.
        }
    }
    ASSERT( found,  "Unkown render state: '" + pValue->NameAsString() + "'." );
} // CPUTRenderStateBlockDX11::ReadValue()

//-----------------------------------------------------------------------------
CPUTResult CPUTRenderStateBlockDX11::ReadProperties(
    CPUTConfigFile                &file,
    const std::string                 &blockName,
    const CPUTRenderStateMapEntry *pMap,
    void                          *pDest
)
{
    CPUTConfigBlock *pProperties = file.GetBlockByName(blockName);
    if( !pProperties )
    {
        // Note: We choose not to assert here.  The nature of the parameter block is that
        // only the values that deviate from default need to be present.  It is very
        // common that blocks will be missing
        return CPUT_ERROR_PARAMETER_BLOCK_NOT_FOUND;
    }

    UINT count = pProperties->ValueCount();
    for( UINT ii=0; ii<count; ii++ )
    {
        // Get the next property
        CPUTConfigEntry *pValue = pProperties->GetValue(ii);
        ASSERT( pValue->IsValid(), "Invalid Value: '"+pValue->NameAsString()+"'." );
        ReadValue( pValue, pMap, pDest );
    }
    return CPUT_SUCCESS;
} // CPUTRenderStateBlockDX11::ReadProperties()

//-----------------------------------------------------------------------------
void CPUTRenderStateDX11::SetDefaults()
{
    // TODO: it would be nice if we could just initialize this with struct initialization.
    UINT ii;
    BlendDesc.AlphaToCoverageEnable = FALSE;
    BlendDesc.IndependentBlendEnable = FALSE;
    for( ii=0; ii<8; ii++ )
        {
        BlendDesc.RenderTarget[ii].BlendEnable    = FALSE;
        BlendDesc.RenderTarget[ii].SrcBlend       = D3D11_BLEND_ONE;
        BlendDesc.RenderTarget[ii].DestBlend      = D3D11_BLEND_ZERO;
        BlendDesc.RenderTarget[ii].BlendOp        = D3D11_BLEND_OP_ADD;
        BlendDesc.RenderTarget[ii].SrcBlendAlpha  = D3D11_BLEND_ONE;
        BlendDesc.RenderTarget[ii].DestBlendAlpha = D3D11_BLEND_ZERO;
        BlendDesc.RenderTarget[ii].BlendOpAlpha   = D3D11_BLEND_OP_ADD;
        BlendDesc.RenderTarget[ii].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    }

    BlendFactor[0] = BlendFactor[1] = BlendFactor[2] = BlendFactor[3] = 1.0f;
    SampleMask = 0xFFFFFFFF;

    DepthStencilDesc.DepthEnable                  = TRUE;
    DepthStencilDesc.DepthWriteMask               = D3D11_DEPTH_WRITE_MASK_ALL;
    DepthStencilDesc.DepthFunc                    = D3D11_COMPARISON_GREATER_EQUAL;
    DepthStencilDesc.StencilEnable                = FALSE;
    DepthStencilDesc.StencilReadMask              = D3D11_DEFAULT_STENCIL_READ_MASK;
    DepthStencilDesc.StencilWriteMask             = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    DepthStencilDesc.FrontFace.StencilFunc        = D3D11_COMPARISON_ALWAYS;
    DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    DepthStencilDesc.FrontFace.StencilPassOp      = D3D11_STENCIL_OP_KEEP;
    DepthStencilDesc.FrontFace.StencilFailOp      = D3D11_STENCIL_OP_KEEP;
    DepthStencilDesc.BackFace.StencilFunc         = D3D11_COMPARISON_ALWAYS;
    DepthStencilDesc.BackFace.StencilDepthFailOp  = D3D11_STENCIL_OP_KEEP;
    DepthStencilDesc.BackFace.StencilPassOp       = D3D11_STENCIL_OP_KEEP;
    DepthStencilDesc.BackFace.StencilFailOp       = D3D11_STENCIL_OP_KEEP;

    RasterizerDesc.FillMode                       = D3D11_FILL_SOLID;
    RasterizerDesc.CullMode                       = D3D11_CULL_BACK;
    RasterizerDesc.FrontCounterClockwise          = FALSE;
    RasterizerDesc.DepthBias                      = 0;
    RasterizerDesc.SlopeScaledDepthBias           = 0.0f;
    RasterizerDesc.DepthBiasClamp                 = 0.0f;
    RasterizerDesc.DepthClipEnable                = TRUE;
    RasterizerDesc.ScissorEnable                  = FALSE;
    RasterizerDesc.MultisampleEnable              = TRUE;
    RasterizerDesc.AntialiasedLineEnable          = FALSE;

    for( ii=0; ii<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ii++ )
    {
        SamplerDesc[ii].Filter                    = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
        SamplerDesc[ii].AddressU                  = D3D11_TEXTURE_ADDRESS_WRAP; // Note that these are different from DX default (*CLAMP)
        SamplerDesc[ii].AddressV                  = D3D11_TEXTURE_ADDRESS_WRAP;
        SamplerDesc[ii].AddressW                  = D3D11_TEXTURE_ADDRESS_WRAP;
        SamplerDesc[ii].MipLODBias                = 0;
        SamplerDesc[ii].MaxAnisotropy             = 1;
        SamplerDesc[ii].ComparisonFunc            = D3D11_COMPARISON_NEVER;
        SamplerDesc[ii].BorderColor[0]            = 0.0f;
        SamplerDesc[ii].BorderColor[1]            = 0.0f;
        SamplerDesc[ii].BorderColor[2]            = 0.0f;
        SamplerDesc[ii].BorderColor[3]            = 0.0f;
        SamplerDesc[ii].MinLOD                    = -FLT_MAX;
        SamplerDesc[ii].MaxLOD                    = FLT_MAX;
    }
} // CPUTRenderStateDX11::SetDefaults()

//-----------------------------------------------------------------------------
CPUTResult CPUTRenderStateBlockDX11::LoadRenderStateBlock(const std::string &fileName)
{
    // TODO: If already loaded, then Release() all the old members

    // use the fileName for now, maybe we'll add names later?
    mMaterialName = fileName;

    // Open/parse the file
    CPUTConfigFile file;
    CPUTResult result = file.LoadFile(fileName);
    ASSERT( !FAILED(result), "Failed loading file: '" + fileName + "'." );
    UNREFERENCED_PARAMETER(result);

    // Note: We ignore "not found error" results for ReadProperties() calls.
    // These blocks are optional.
    UINT ii;
    for( ii=0; ii<8; ii++ )
    {
        char pBlockName[64];
        sprintf( pBlockName, "RenderTargetBlendStateDX11_%d", ii+1 );
        ReadProperties( file, std::string(pBlockName), pRenderTargetBlendDescMap, &mStateDesc.BlendDesc.RenderTarget[ii] );
    }
    ReadProperties( file, "BlendStateDX11",        pBlendDescMap,        &mStateDesc.BlendDesc );
    ReadProperties( file, "DepthStencilStateDX11", pDepthStencilDescMap, &mStateDesc.DepthStencilDesc);
    ReadProperties( file, "RasterizerStateDX11",   pRasterizerDescMap,   &mStateDesc.RasterizerDesc);

    mNumSamplers = 0;
    for( ii=0; ii<D3D11_COMMONSHADER_SAMPLER_SLOT_COUNT; ii++ )
    {
        // TODO: Use sampler names from .fx file.  Already did this for texture names.
        // The challenge is that the renderstate file is independent from the material (and the shaders).
        // Another feature is that the artists don't name the samplers (in the CPUTSL source).  Though, arbitrary .fx files can.
        // TODO: Add sampler-state properties to CPUTSL source (e.g., filter modes).  Then, have ShaderGenerator output a .rs file.
        char pBlockName[64];
        sprintf( pBlockName, "SamplerDX11_%d", ii+1 );
        CPUTResult result = ReadProperties( file, std::string(pBlockName), pSamplerDescMap, &mStateDesc.SamplerDesc[ii] );
        if( CPUT_SUCCESS != result )
        {
            break; // Reached last sampler spec
        }
        ++mNumSamplers;
    }
    CreateNativeResources();

    return CPUT_SUCCESS;
} // CPUTRenderStateBlockDX11::LoadRenderStateBlock()

//-----------------------------------------------------------------------------
void CPUTRenderStateBlockDX11::CreateNativeResources()
{
    // Now, create the DX render state items
    ID3D11Device *pDevice = CPUT_DX11::GetDevice();
    HRESULT hr;
    UNREFERENCED_PARAMETER(hr);

    hr = pDevice->CreateBlendState( &mStateDesc.BlendDesc, &mpBlendState );
    ASSERT( SUCCEEDED(hr), "Failed to create blend state." );

    hr = pDevice->CreateDepthStencilState( &mStateDesc.DepthStencilDesc, &mpDepthStencilState );
    ASSERT( SUCCEEDED(hr), "Failed to create depth stencil state." );

    hr = pDevice->CreateRasterizerState( &mStateDesc.RasterizerDesc, &mpRasterizerState );
    ASSERT( SUCCEEDED(hr), "Failed to create rasterizer state." );

    // TODO: how to map samplers to shaders?
    // Each type can have different samplers assigned (VS, PS, GS, etc.)
    // How does DX treat them?  16 unified?  or 16 each?
    // For now, just read 16 samplers, and set to all stages

    for( UINT ii=0; ii<mNumSamplers; ii++ )
    {
        hr = pDevice->CreateSamplerState( &mStateDesc.SamplerDesc[ii], &mpSamplerState[ii] );
        ASSERT( SUCCEEDED(hr), "Failed to create sampler state." );
    }
} // CPUTRenderStateBlockDX11::CreateDXResources()

//-----------------------------------------------------------------------------
void CPUTRenderStateBlockDX11::SetRenderStates(  )
{
    ID3D11DeviceContext *pContext = CPUT_DX11::GetContext();

    pContext->OMSetBlendState( mpBlendState, mStateDesc.BlendFactor, mStateDesc.SampleMask );
    pContext->OMSetDepthStencilState( mpDepthStencilState, 0 ); // TODO: read stecil ref from config file
    pContext->RSSetState( mpRasterizerState );
    pContext->PSSetSamplers( 0, mNumSamplers, mpSamplerState );
    pContext->VSSetSamplers( 0, mNumSamplers, mpSamplerState );
    pContext->GSSetSamplers( 0, mNumSamplers, mpSamplerState );
} // CPUTRenderStateBlockDX11::SetRenderState()

CPUTRenderStateBlockDX11* CPUTRenderStateBlockDX11::Create()
{
	return new CPUTRenderStateBlockDX11();
}
void SetRenderStateBlock(CPUTRenderStateBlock* pNewBase, CPUTRenderStateBlock* pCurrentBase)
{
    if (pNewBase == pCurrentBase)
        return;
    CPUTRenderStateBlockDX11* pNew = (CPUTRenderStateBlockDX11*)pNewBase;
    CPUTRenderStateBlockDX11* pCurrent = (CPUTRenderStateBlockDX11*)pCurrentBase;
    ID3D11DeviceContext* pContext = CPUT_DX11::GetContext();
    ID3D11BlendState* pNewBS = pNew->GetBlendState();
    ID3D11DepthStencilState* pNewDS = pNew->GetDepthStencilState();
    ID3D11RasterizerState* pNewRS = pNew->GetRasterizerState();
    ID3D11SamplerState** pNewSS = pNew->GetSamplerState();
    int newNumSamplers = pNew->GetNumSamplers();;

    ID3D11BlendState* pCurrentBS = (ID3D11BlendState*)-1;
    ID3D11DepthStencilState* pCurrentDS = (ID3D11DepthStencilState*)-1;
    ID3D11RasterizerState* pCurrentRS = (ID3D11RasterizerState*)-1;
    ID3D11SamplerState** pCurrentSS = NULL;
    float pCurrentBlendFactor[] = { 1.0, 1.0, 1.0, 1.0 };
    INT currentStencilRef = -1;
    UINT currentSampleMask = 0xffffffff;
    int currentNumSamplers = 0;
    if (pCurrent)
    {
        pCurrentBS = pCurrent->GetBlendState();
        pCurrentDS = pCurrent->GetDepthStencilState();
        pCurrentRS = pCurrent->GetRasterizerState();
        pCurrentSS = pCurrent->GetSamplerState();
        currentNumSamplers = pCurrent->GetNumSamplers();
        float* tempBlend = pCurrent->GetBlendFactor();
        pCurrentBlendFactor[0] = tempBlend[0];
        pCurrentBlendFactor[1] = tempBlend[1];
        pCurrentBlendFactor[2] = tempBlend[2];
        pCurrentBlendFactor[3] = tempBlend[3];
        currentStencilRef = pCurrent->GetStencilRef();
        currentSampleMask = pCurrent->GetSampleMask();
    }

    UINT sampleMask = pNew->GetSampleMask();
    float* blendFactor = pNew->GetBlendFactor();
    if (pNewBS != pCurrentBS
        || sampleMask != currentSampleMask
        || blendFactor[0] != pCurrentBlendFactor[0]
        || blendFactor[1] != pCurrentBlendFactor[1]
        || blendFactor[2] != pCurrentBlendFactor[2]
        || blendFactor[3] != pCurrentBlendFactor[3]
        )
    {
        pContext->OMSetBlendState(pNewBS, blendFactor, sampleMask);
    }
    UINT stencilRef = pNew->GetStencilRef();
    if (pNewDS != pCurrentDS || stencilRef != currentStencilRef)
    {
        pContext->OMSetDepthStencilState(pNewDS, stencilRef);
    }
    if (pNewRS != pCurrentRS)
    {
        pContext->RSSetState(pNewRS);
    }
    bool setSamplers = newNumSamplers != currentNumSamplers;
    if (!setSamplers)
    {
        for (int sampler = 0; sampler < newNumSamplers; sampler++)
        {
            if (pNewSS[sampler] != pCurrentSS[sampler])
            {
                setSamplers = true;
                break;
            }
        }
    }
    if (setSamplers)
    {
        //TODO: move to material. use sampler names
        //match to shader stage
        pContext->PSSetSamplers(0, newNumSamplers, pNewSS);
        pContext->VSSetSamplers(0, newNumSamplers, pNewSS);
        pContext->GSSetSamplers(0, newNumSamplers, pNewSS);
    }

}



