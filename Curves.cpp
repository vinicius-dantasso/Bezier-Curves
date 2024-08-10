/**********************************************************************************
// Curves (Código Fonte)
//
// Criação:     12 Ago 2020
// Atualização: 06 Ago 2023
// Compilador:  Visual C++ 2022
//
// Descrição:   Base para gerar curvas usando Corner-Cutting
//
**********************************************************************************/

#include "Curves.h"

// ------------------------------------------------------------------------------

void Curves::Init()
{
    graphics->ResetCommands();

    // ---------[ Build Geometry ]------------
    
    // tamanho do buffer de vértices em bytes
    const uint vbSizeCurve  = MaxCurve * sizeof(Vertex);
    const uint vbSizeCtrl   = MaxCtrl * sizeof(Vertex);
    const uint vbSizeSquare = MaxSquareVertex * sizeof(Vertex);

    // cria malha 3D
    ctrlPoint1      = new Mesh(vbSizeCtrl, sizeof(Vertex));
    ctrlPoint2      = new Mesh(vbSizeCtrl, sizeof(Vertex));
    curve           = new Mesh(vbSizeCurve, sizeof(Vertex));
    finalCurve      = new Mesh(vbSizeCurve, sizeof(Vertex));
    squarePoint1    = new Mesh(vbSizeSquare, sizeof(Vertex));
    squarePoint2    = new Mesh(vbSizeSquare, sizeof(Vertex));
    squarePoint3    = new Mesh(vbSizeSquare, sizeof(Vertex));
    squarePoint4    = new Mesh(vbSizeSquare, sizeof(Vertex));

    // ---------------------------------------

    BuildRootSignature();
    BuildPipelineState();        
    
    // ---------------------------------------

    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

void Curves::Update()
{
    cx = float(window->CenterX());
    cy = float(window->CenterY());
    mx = float(input->MouseX());
    my = float(input->MouseY());

    // sai com o pressionamento da tecla ESC
    if (input->KeyPress(VK_ESCAPE))
        window->Close();

    if (input->KeyPress(VK_DELETE))
        DeleteCurve();

    if (input->KeyPress('S'))
        SaveCurve();

    if (input->KeyPress('L'))
        LoadCurve();

    // Cria vértices com o botão do mouse
    CreateVertices();
    DrawVertices();

    Display();
}

// ------------------------------------------------------------------------------

// Cria os pontos de apoio a partir do clique do mouse
void Curves::CreateVertices()
{
    float x = (mx - cx) / cx;
    float y = (cy - my) / cy;

    if (input->KeyPress(VK_LBUTTON))
    {
        switch (clickCount)
        {
            case 0:
                ++clickCount;
                index = 0;

                ctrl1[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
                index = (index + 1) % MaxCtrl;

                ctrl1[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
                index = (index + 1) % MaxCtrl;

                ctrl1[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::DarkRed) };

                ctrlCount1 += 3;
                fix = true;
            break;

            case 1:
                ++clickCount;
            break;

            case 2:
                ++clickCount;
                index = 0;

                ctrl2[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
                index = (index + 1) % MaxCtrl;

                ctrl2[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
                index = (index + 1) % MaxCtrl;

                ctrl2[index] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::DarkRed) };
                ctrlCount2 += 3;
                
                ++totalCurves;
                createCurve = true;
                fix = true;
            break;

            case 3:
                ++clickCount;
            break;

            case 4:
                newCurve = true;
                canDraw = true;
                clickCount = 2;
            break;
        }
    }

    if (createCurve)
        CreateCurve();

    DrawSquares();

    if(totalCurves >= 1)
        DrawCurve();
}

// ------------------------------------------------------------------------------

// Desenha os pontos de apoio
void Curves::DrawVertices()
{
    float x = (mx - cx) / cx;
    float y = (cy - my) / cy;

    if (clickCount == 1)
    {
        float xx = (ctrl1[1].Pos.x - x) + ctrl1[1].Pos.x;
        float yy = (ctrl1[1].Pos.y - y) + ctrl1[1].Pos.y;

        ctrl1[0] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
        ctrl1[2] = { XMFLOAT3(xx, yy, 0.0f), XMFLOAT4(Colors::Red) };
    }
    else if (clickCount >= 2 && clickCount < 4)
    {
        float xx = (ctrl2[1].Pos.x - x) + ctrl2[1].Pos.x;
        float yy = (ctrl2[1].Pos.y - y) + ctrl2[1].Pos.y;

        ctrl2[0] = { XMFLOAT3(x, y, 0.0f), XMFLOAT4(Colors::Red) };
        ctrl2[2] = { XMFLOAT3(xx, yy, 0.0f), XMFLOAT4(Colors::Red) };
    }

    // copia vértices para o buffer da GPU usando o buffer de Upload
    graphics->ResetCommands();
    graphics->Copy(ctrl1, ctrlPoint1->vertexBufferSize, ctrlPoint1->vertexBufferUpload, ctrlPoint1->vertexBufferGPU);
    graphics->SubmitCommands();

    graphics->ResetCommands();
    graphics->Copy(ctrl2, ctrlPoint2->vertexBufferSize, ctrlPoint2->vertexBufferUpload, ctrlPoint2->vertexBufferGPU);
    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

// Cria uma curva
void Curves::CreateCurve()
{
    float segments = 50;
    float t;

    for (int i = 0; i < segments; i++)
    {
        t = i / float(segments - 1);
        float alpha = pow((1 - t), 3);
        float beta = 3 * t * pow((1 - t), 2);
        float gama = 3 * pow(t, 2) * (1 - t);
        float teta = pow(t, 3);

        XMFLOAT3 point =
        {
            alpha * ctrl1[1].Pos.x + beta * ctrl1[2].Pos.x + gama * ctrl2[2].Pos.x + teta * ctrl2[1].Pos.x,
            alpha * ctrl1[1].Pos.y + beta * ctrl1[2].Pos.y + gama * ctrl2[2].Pos.y + teta * ctrl2[1].Pos.y,
            0.0f
        };

        curvePoints[curveIndex] = { point, XMFLOAT4(Colors::Yellow) };
        curveIndex = (curveIndex + 1) % 50;

        if (curveCount < 50)
            ++curveCount;
    }

    // Caso seja hora de criar uma nova curva
    if (newCurve)
    {
        newCurve = false;
        createCurve = false;

        curveIndex = 50 * (totalCurves - 1);
        for (int i = 0; i < segments; i++)
        {
            showCurves[curveIndex] = curvePoints[i];
            curveIndex = (curveIndex + 1) % (50 * totalCurves);

            if(curveCount2 < 50 * totalCurves)
                ++curveCount2;
        }

        curveIndex = 50 * totalCurves;
        curveCount = 0;

        XMFLOAT3 newPoint = {
            (ctrl2[1].Pos.x - ctrl2[2].Pos.x) + ctrl2[1].Pos.x,
            (ctrl2[1].Pos.y - ctrl2[2].Pos.y) + ctrl2[1].Pos.y,
            0.0f
        };

        ctrl1[0] = ctrl2[0];
        ctrl1[1] = ctrl2[1];
        ctrl1[2] = { newPoint, XMFLOAT4(Colors::Red) };
        ctrlCount2 = 0;
    }

    graphics->ResetCommands();
    graphics->Copy(curvePoints, curve->vertexBufferSize, curve->vertexBufferUpload, curve->vertexBufferGPU);
    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

// Salva as informações da curva em um arquivo binário
void Curves::SaveCurve()
{
    ofstream fout("saveCurve.bin", ios::binary);

    saveCurve = true;
    if (fout.is_open())
    {
        // Salvando ctrl1 e ctrl2
        fout.write((char*)&ctrlCount1, sizeof(ctrlCount1));
        fout.write((char*)ctrl1, ctrlCount1 * sizeof(Vertex));

        fout.write((char*)&ctrlCount2, sizeof(ctrlCount2));
        fout.write((char*)ctrl2, ctrlCount2 * sizeof(Vertex));

        // Salvando curvePoints e showCurves
        fout.write((char*)&curveCount, sizeof(curveCount));
        fout.write((char*)curvePoints, curveCount * sizeof(Vertex));
        
        fout.write((char*)&curveCount2, sizeof(curveCount2));
        fout.write((char*)showCurves, curveCount2 * sizeof(Vertex));

        // Salvando Squares
        fout.write((char*)&back1, MaxSquareVertex * sizeof(Vertex));
        fout.write((char*)&back2, MaxSquareVertex * sizeof(Vertex));
        fout.write((char*)&back3, MaxSquareVertex * sizeof(Vertex));
        fout.write((char*)&back4, MaxSquareVertex * sizeof(Vertex));

        // Salvando outras variáveis
        fout.write((char*)&index, sizeof(index));
        fout.write((char*)&clickCount, sizeof(clickCount));
        fout.write((char*)&curveIndex, sizeof(curveIndex));
        fout.write((char*)&totalCurves, sizeof(totalCurves));

        // Salvando variáveis bool
        fout.write((char*)&newCurve, sizeof(newCurve));
        fout.write((char*)&createCurve, sizeof(createCurve));
        fout.write((char*)&canDraw, sizeof(canDraw));
        fout.write((char*)&fix, sizeof(fix));
        fout.write((char*)&erase, sizeof(erase));
        
        fout.close();
    }
}

// ------------------------------------------------------------------------------

// Carrega as informações de uma curva salva de um arquivo binário
void Curves::LoadCurve()
{
    ifstream fin("saveCurve.bin", ios::binary);

    loadCurve = true;
    if (fin.is_open())
    {
        // Lendo ctrl1 e ctrl2
        fin.read((char*)&ctrlCount1, sizeof(ctrlCount1));
        fin.read((char*)ctrl1, ctrlCount1 * sizeof(Vertex));

        fin.read((char*)&ctrlCount2, sizeof(ctrlCount2));
        fin.read((char*)ctrl2, ctrlCount2 * sizeof(Vertex));

        // Lendo curvePoints e showCurves
        fin.read((char*)&curveCount, sizeof(curveCount));
        fin.read((char*)curvePoints, curveCount * sizeof(Vertex));
        
        fin.read((char*)&curveCount2, sizeof(curveCount2));
        fin.read((char*)showCurves, curveCount2 * sizeof(Vertex));

        // Lendo Squares
        fin.read((char*)&back1, MaxSquareVertex * sizeof(Vertex));
        fin.read((char*)&back2, MaxSquareVertex * sizeof(Vertex));
        fin.read((char*)&back3, MaxSquareVertex * sizeof(Vertex));
        fin.read((char*)&back4, MaxSquareVertex * sizeof(Vertex));

        // Lendo outras variáveis
        fin.read((char*)&index, sizeof(index));
        fin.read((char*)&clickCount, sizeof(clickCount));
        fin.read((char*)&curveIndex, sizeof(curveIndex));
        fin.read((char*)&totalCurves, sizeof(totalCurves));

        // Lendo variáveis bool
        fin.read((char*)&newCurve, sizeof(newCurve));
        fin.read((char*)&createCurve, sizeof(createCurve));
        fin.read((char*)&canDraw, sizeof(canDraw));
        fin.read((char*)&fix, sizeof(fix));
        fin.read((char*)&erase, sizeof(erase));

        fin.close();
    }
}

// ------------------------------------------------------------------------------

// Deleta uma curva já desenhada
void Curves::DeleteCurve()
{
    erase = true;
    memset(curvePoints, 0, sizeof(curvePoints));
    memset(showCurves, 0, sizeof(showCurves));
    memset(ctrl1, 0, sizeof(ctrl1));
    memset(ctrl2, 0, sizeof(ctrl2));
    ctrlCount1 = 0;
    ctrlCount2 = 0;
    index = 0;
    clickCount = 0;
    curveCount = 0;
    curveCount2 = 0;
    curveIndex = 0;
    totalCurves = 0;
    newCurve = false;
    createCurve = false;
    canDraw = false;
    fix = false;
}

// ------------------------------------------------------------------------------

void Curves::DrawCurve()
{
    graphics->ResetCommands();
    graphics->Copy(showCurves, finalCurve->vertexBufferSize, finalCurve->vertexBufferUpload, finalCurve->vertexBufferGPU);
    graphics->SubmitCommands();
}

// ------------------------------------------------------------------------------

// Desenho quadrados dos pontos de apoio
void Curves::DrawSquares()
{
    float x = (mx - cx) / cx;
    float y = (cy - my) / cy;
    float xx = 0.0f;
    float yy = 0.0f;

    if (clickCount == 1)
    {
        xx = (ctrl1[1].Pos.x - x) + ctrl1[1].Pos.x;
        yy = (ctrl1[1].Pos.y - y) + ctrl1[1].Pos.y;

        Vertex square1[MaxSquareVertex] =
        {
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
        };

        memcpy(back1, square1, sizeof(square1));

        graphics->ResetCommands();
        graphics->Copy(square1, squarePoint1->vertexBufferSize, squarePoint1->vertexBufferUpload, squarePoint1->vertexBufferGPU);
        graphics->SubmitCommands();

        if (fix)
        {
            fix = false;
            Vertex square3[MaxSquareVertex] =
            {
                { XMFLOAT3(x - 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x + 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x + 0.02f, y + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x - 0.02f, y + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x - 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
            };

            memcpy(back3, square3, sizeof(square3));

            graphics->ResetCommands();
            graphics->Copy(square3, squarePoint3->vertexBufferSize, squarePoint3->vertexBufferUpload, squarePoint3->vertexBufferGPU);
            graphics->SubmitCommands();
        }
    }
    else if (clickCount == 3)
    {
        xx = (ctrl2[1].Pos.x - x) + ctrl2[1].Pos.x;
        yy = (ctrl2[1].Pos.y - y) + ctrl2[1].Pos.y;

        Vertex square2[MaxSquareVertex] =
        {
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
        };

        memcpy(back2, square2, sizeof(square2));

        graphics->ResetCommands();
        graphics->Copy(square2, squarePoint2->vertexBufferSize, squarePoint2->vertexBufferUpload, squarePoint2->vertexBufferGPU);
        graphics->SubmitCommands();

        if (fix)
        {
            fix = false;
            Vertex square4[MaxSquareVertex] =
            {
                { XMFLOAT3(x - 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x + 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x + 0.02f, y + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x - 0.02f, y + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
                { XMFLOAT3(x - 0.02f, y - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
            };

            memcpy(back4, square4, sizeof(square4));

            graphics->ResetCommands();
            graphics->Copy(square4, squarePoint4->vertexBufferSize, squarePoint4->vertexBufferUpload, squarePoint4->vertexBufferGPU);
            graphics->SubmitCommands();
        }
    }
    else if(canDraw)
    {
        canDraw = false;
        xx = ctrl1[1].Pos.x;
        yy = ctrl1[1].Pos.y;

        Vertex square1[MaxSquareVertex] =
        {
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
        };

        memcpy(back1, square1, sizeof(square1));

        graphics->ResetCommands();
        graphics->Copy(square1, squarePoint1->vertexBufferSize, squarePoint1->vertexBufferUpload, squarePoint1->vertexBufferGPU);
        graphics->SubmitCommands();

        xx = ctrl1[2].Pos.x;
        yy = ctrl1[2].Pos.y;

        Vertex square3[MaxSquareVertex] =
        {
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx + 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy + 0.02f, 0.0f), XMFLOAT4(Colors::Red) },
            { XMFLOAT3(xx - 0.02f, yy - 0.02f, 0.0f), XMFLOAT4(Colors::Red) }
        };

        memcpy(back3, square3, sizeof(square3));

        graphics->ResetCommands();
        graphics->Copy(square3, squarePoint3->vertexBufferSize, squarePoint3->vertexBufferUpload, squarePoint3->vertexBufferGPU);
        graphics->SubmitCommands();

        Vertex square2[MaxSquareVertex] = {};
        memcpy(back2, square2, sizeof(square2));

        graphics->ResetCommands();
        graphics->Copy(square2, squarePoint2->vertexBufferSize, squarePoint2->vertexBufferUpload, squarePoint2->vertexBufferGPU);
        graphics->SubmitCommands();

        Vertex square4[MaxSquareVertex] = {};
        memcpy(back4, square4, sizeof(square4));

        graphics->ResetCommands();
        graphics->Copy(square4, squarePoint4->vertexBufferSize, squarePoint4->vertexBufferUpload, squarePoint4->vertexBufferGPU);
        graphics->SubmitCommands();
    }

    if (erase)
    {
        erase = false;

        Vertex square1[MaxSquareVertex] = {};
        Vertex square2[MaxSquareVertex] = {};
        Vertex square3[MaxSquareVertex] = {};
        Vertex square4[MaxSquareVertex] = {};

        OutputDebugString("Entrei aqui");

        graphics->ResetCommands();
        graphics->Copy(square1, squarePoint1->vertexBufferSize, squarePoint1->vertexBufferUpload, squarePoint1->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(square2, squarePoint2->vertexBufferSize, squarePoint2->vertexBufferUpload, squarePoint2->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(square3, squarePoint3->vertexBufferSize, squarePoint3->vertexBufferUpload, squarePoint3->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(square4, squarePoint4->vertexBufferSize, squarePoint4->vertexBufferUpload, squarePoint4->vertexBufferGPU);
        graphics->SubmitCommands();
    }

    if (loadCurve)
    {
        loadCurve = false;

        graphics->ResetCommands();
        graphics->Copy(back1, squarePoint1->vertexBufferSize, squarePoint1->vertexBufferUpload, squarePoint1->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(back2, squarePoint2->vertexBufferSize, squarePoint2->vertexBufferUpload, squarePoint2->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(back3, squarePoint3->vertexBufferSize, squarePoint3->vertexBufferUpload, squarePoint3->vertexBufferGPU);
        graphics->SubmitCommands();

        graphics->ResetCommands();
        graphics->Copy(back4, squarePoint4->vertexBufferSize, squarePoint4->vertexBufferUpload, squarePoint4->vertexBufferGPU);
        graphics->SubmitCommands();
    }
}

// ------------------------------------------------------------------------------

void Curves::Display()
{
    // limpa backbuffer
    graphics->Clear(pipelineState);

    // Desenhar vertices
    graphics->CommandList()->SetGraphicsRootSignature(rootSignature);
    graphics->CommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINESTRIP);

    graphics->CommandList()->IASetVertexBuffers(0, 1, ctrlPoint1->VertexBufferView());
    graphics->CommandList()->DrawInstanced(ctrlCount1, 1, 0, 0);

    graphics->CommandList()->IASetVertexBuffers(0, 1, ctrlPoint2->VertexBufferView());
    graphics->CommandList()->DrawInstanced(ctrlCount2, 1, 0, 0);

    // Desenhar curva durante criação
    graphics->CommandList()->IASetVertexBuffers(0, 1, curve->VertexBufferView());
    graphics->CommandList()->DrawInstanced(curveCount, 1, 0, 0);

    // Desenhar curva final
    graphics->CommandList()->IASetVertexBuffers(0, 1, finalCurve->VertexBufferView());
    graphics->CommandList()->DrawInstanced(curveCount2, 1, 0, 0);

    // Desenhar os pontos de ancoragem
    graphics->CommandList()->IASetVertexBuffers(0, 1, squarePoint1->VertexBufferView());
    graphics->CommandList()->DrawInstanced(MaxSquareVertex, 1, 0, 0);

    graphics->CommandList()->IASetVertexBuffers(0, 1, squarePoint2->VertexBufferView());
    graphics->CommandList()->DrawInstanced(MaxSquareVertex, 1, 0, 0);

    graphics->CommandList()->IASetVertexBuffers(0, 1, squarePoint3->VertexBufferView());
    graphics->CommandList()->DrawInstanced(MaxSquareVertex, 1, 0, 0);

    graphics->CommandList()->IASetVertexBuffers(0, 1, squarePoint4->VertexBufferView());
    graphics->CommandList()->DrawInstanced(MaxSquareVertex, 1, 0, 0);

    // apresenta backbuffer
    graphics->Present();    
}

// ------------------------------------------------------------------------------

void Curves::Finalize()
{
    rootSignature->Release();
    pipelineState->Release();
    delete ctrlPoint1;
    delete ctrlPoint2;
    delete curve;
    delete finalCurve;
    delete squarePoint1;
    delete squarePoint2;
    delete squarePoint3;
    delete squarePoint4;
}

// ------------------------------------------------------------------------------
//                                     D3D                                      
// ------------------------------------------------------------------------------

void Curves::BuildRootSignature()
{
    // descrição para uma assinatura vazia
    D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
    rootSigDesc.NumParameters = 0;
    rootSigDesc.pParameters = nullptr;
    rootSigDesc.NumStaticSamplers = 0;
    rootSigDesc.pStaticSamplers = nullptr;
    rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    // serializa assinatura raiz
    ID3DBlob* serializedRootSig = nullptr;
    ID3DBlob* error = nullptr;

    ThrowIfFailed(D3D12SerializeRootSignature(
        &rootSigDesc,
        D3D_ROOT_SIGNATURE_VERSION_1,
        &serializedRootSig,
        &error));

    // cria uma assinatura raiz vazia
    ThrowIfFailed(graphics->Device()->CreateRootSignature(
        0,
        serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(),
        IID_PPV_ARGS(&rootSignature)));
}

// ------------------------------------------------------------------------------

void Curves::BuildPipelineState()
{
    // --------------------
    // --- Input Layout ---
    // --------------------
    
    D3D12_INPUT_ELEMENT_DESC inputLayout[2] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // --------------------
    // ----- Shaders ------
    // --------------------

    ID3DBlob* vertexShader;
    ID3DBlob* pixelShader;

    D3DReadFileToBlob(L"Shaders/Vertex.cso", &vertexShader);
    D3DReadFileToBlob(L"Shaders/Pixel.cso", &pixelShader);

    // --------------------
    // ---- Rasterizer ----
    // --------------------

    D3D12_RASTERIZER_DESC rasterizer = {};
    rasterizer.FillMode = D3D12_FILL_MODE_WIREFRAME;
    rasterizer.CullMode = D3D12_CULL_MODE_NONE;
    rasterizer.FrontCounterClockwise = FALSE;
    rasterizer.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer.DepthClipEnable = TRUE;
    rasterizer.MultisampleEnable = FALSE;
    rasterizer.AntialiasedLineEnable = FALSE;
    rasterizer.ForcedSampleCount = 0;
    rasterizer.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    // ---------------------
    // --- Color Blender ---
    // ---------------------

    D3D12_BLEND_DESC blender = {};
    blender.AlphaToCoverageEnable = FALSE;
    blender.IndependentBlendEnable = FALSE;
    const D3D12_RENDER_TARGET_BLEND_DESC defaultRenderTargetBlendDesc =
    {
        FALSE,FALSE,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
        D3D12_LOGIC_OP_NOOP,
        D3D12_COLOR_WRITE_ENABLE_ALL,
    };
    for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i)
        blender.RenderTarget[i] = defaultRenderTargetBlendDesc;

    // ---------------------
    // --- Depth Stencil ---
    // ---------------------

    D3D12_DEPTH_STENCIL_DESC depthStencil = {};
    depthStencil.DepthEnable = TRUE;
    depthStencil.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    depthStencil.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
    depthStencil.StencilEnable = FALSE;
    depthStencil.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
    depthStencil.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    const D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =
    { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };
    depthStencil.FrontFace = defaultStencilOp;
    depthStencil.BackFace = defaultStencilOp;
    
    // -----------------------------------
    // --- Pipeline State Object (PSO) ---
    // -----------------------------------

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso = {};
    pso.pRootSignature = rootSignature;
    pso.VS = { reinterpret_cast<BYTE*>(vertexShader->GetBufferPointer()), vertexShader->GetBufferSize() };
    pso.PS = { reinterpret_cast<BYTE*>(pixelShader->GetBufferPointer()), pixelShader->GetBufferSize() };
    pso.BlendState = blender;
    pso.SampleMask = UINT_MAX;
    pso.RasterizerState = rasterizer;
    pso.DepthStencilState = depthStencil;
    pso.InputLayout = { inputLayout, 2 };
    pso.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
    pso.NumRenderTargets = 1;
    pso.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    pso.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    pso.SampleDesc.Count = graphics->Antialiasing();
    pso.SampleDesc.Quality = graphics->Quality();
    graphics->Device()->CreateGraphicsPipelineState(&pso, IID_PPV_ARGS(&pipelineState));

    vertexShader->Release();
    pixelShader->Release();
}

// ------------------------------------------------------------------------------
//                                  WinMain                                      
// ------------------------------------------------------------------------------

int APIENTRY WinMain(_In_ HINSTANCE hInstance,    _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
{
    try
    {
        // cria motor e configura a janela
        Engine* engine = new Engine();
        engine->window->Mode(WINDOWED);
        engine->window->Size(980, 960);
        engine->window->ResizeMode(ASPECTRATIO);
        engine->window->Color(0, 0, 0);
        engine->window->Title("Curves");
        engine->window->Icon(IDI_ICON);
        engine->window->LostFocus(Engine::Pause);
        engine->window->InFocus(Engine::Resume);

        // cria e executa a aplicação
        engine->Start(new Curves());

        // finaliza execução
        delete engine;
    }
    catch (Error & e)
    {
        // exibe mensagem em caso de erro
        MessageBox(nullptr, e.ToString().data(), "Curves", MB_OK);
    }

    return 0;
}

// ----------------------------------------------------------------------------
