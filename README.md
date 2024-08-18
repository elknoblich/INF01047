# INF01047: Trabalho Final

<p align="justify">
    Durante o desenvolvimento do trabalho final, os integrantes fizeram o trabalho final tanto individualmente quanto em grupo, <i>pair programming</i>, em sala da aula nos dias das apresentações finais e parciais. Logo, o histórico <i>commit</i> não é uma boa indicação do que cada integrante fez no trabalho.
</p>

<ul>
    <li>
        Erick: criação do repositório, renderização do tubarão, <i>triggers</i> do tuburão a partir dos botões, criação do relatório.
    </li>
    <li>
        Nicolas: reutilização da câmera livre do laboratório dois, interação com o botão esquerdo do mouse, renderização do plano, renderização dos botões.
    </li>
</ul>

<p align="justify">
    Ferramentas como ChatGPT, Github Copilot e OpenAI Codex não foram utilizadas durante o desenvolvimento no trabalho para evitar "cola". Entretanto, aproveitamos os códigos já desenvolvidos nos laboratórios anteriores, como a câmera de movimento livre.
</p>

<p align="justify">
    O desenvolvimento do trabalho deu-se de acordo com os requisitos mínimos, ou seja, colocamos cada item dos requisitos mínimos separadamente e só depois montamos as interações. Por exemplo, o modelo do tubarão foi escolhido simplesmente por ele ser suficientemente complexo e só depois dele ser corretamente renderizado na tela é que foi feito seu carregamento e descarregamento de acordo com o <i>trigger</i> do botão, e antes disso tudo ocorrer o mesmo processo foi feito com o botão, etc. 
</p>

<p align="justify">
    A aplicação consiste numa câmera livre <i>WSDA</i> do laboratório dois juntamente com interação do usuário com o cenário a partir do botão esquerdo do mouse, que é utilizado para apertar botões. Esses botões executam os requisitos mínimos do trabalho.
</p>

<p align="justify">
    O trabalho final pode ser configurado de acordo com o arquivo <i>makefile</i> disponível na raiz. Caso algum erro ocorra, será necessário instalar as bibliotecas necessárias. Entretanto, isso varia de máquina para máquina e durante o desenvolvimento do trabalho, os integrantes usaram comandos de compilação diferentes, uma vez que nem todas as biliotecas chamadas eram necessárias e às vezes a inclusão duma fazia com que o trabalho não compilasse. O comando <i>makefile</i> disponibilizado é o mesmo dos laboratórios passados. Após o uso do arquivo <i>makefile</i>, basta executar o programa a partir do executável <i>main.exe</i>.
</p>