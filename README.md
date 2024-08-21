# INF01047: Trabalho Final

## Contribuições
<p align="justify">
    Durante o desenvolvimento do trabalho final, os integrantes fizeram o trabalho final tanto individualmente quanto na modalidade <i>pair programming</i>, em sala da aula nos dias das apresentações finais e parciais. As contribuição de cada membro para o desenvolvimento da aplicação foram as seguintes:
</p>

<div align="justify">
    <ul>
        <li>
            Erick: criação do repositório, renderização dos objetos inciais, <i>triggers</i> de interação do raio e da esfera,
            função para criar curva de bézier e animação com a bézier.
        </li>
        <li>
            Nicolas: o resto.
        </li>
    </ul>
</div>

## IA
<p align="justify">
<ol>
    <p align="justify">
    Ferramentas de IA como ChatGPT foram utilizdas principalmente como:
    </p>
    
<li><strong>Depuração</strong>: certos erros de implementação, compilação ou de execução foram facilmente corrigidos com auxílio de uma ferramenta de IA entregando o contexto (o código) e os passos para a reprodução do problema.
<li><strong>Refatoração</strong>:  IA auxiliou muito para dar sugestões de refatoração do código. 
<li><strong>Trabalho bruto</strong>: certas partes do código, para poupar tempo, foram pedidas para uma IA serem feitas. Mais exatamente a parte que cria as AABB e as matrizes de modelagem dos objetos que são intanciados diversas vezes(funções create_fishes e create_seaweed_models). 
<li><strong>Não sei mais o que fazer</strong>: tal qual o nome deste tópico, certo momentos não era possível escrever o código devido à falta de conhecimento (ou provavelmente falta de tempo para continuar pensando) ou falta de material na bibliografia consultada para desenvolver alguns algoritmos (citados no arquivo com as colisões). Desta forma, a IA deu respostas diretas para implementação de algumas partes, como uma resposta de colisão melhor, a atualização correta da AABB e o calculo do vetor que o tubarão está olhando. 
      <p align="justify">
    Em suma, a IA foi útil para o desenvolvimento do trabalho. Contudo, vale ressaltar que a sua utilidade depende muito do contexto do que já está feito, a exemplo de uma depuração: realizando hipóteses sobre o que pode estar ocasionando o problema. Ou seja, é muito funcional e felizmente não tem a capacidade de excluir a compreensão do que está sendo desenvolvido.
     </p>
</ol>
</p>

## Desenvolvimento
<p align="justify">
    O desenvolvimento do trabalho deu-se inicialmente de acordo com os requisitos mínimos, ou seja, cada item dos requisitos mínimos foi desenvolvido separadamente e só depois as peças foram juntadas para formar um todo coeso. Primeiro foi criado uma arena de teste para realizar a movimentação, basicamente pronta do laboratório 2. Depois foi adicionado os testes e respostas de colisão na arena. As colisões foram implementadas seguindo uma mistura de material da bibliografia consultada, graças a isto, não foi muito difícil fazer os testes de intersecção. A resposta a colisão, porém, fora mais problemático. Calcular a normal da face da AABB em que a colisão está ocorrendo e decompor na direção oposta a velocidade do movimento de um forma suave não foi possível de ser perfeitamente implementado, exceto para a versão final que apenas possui colisão no chão.
</p>
<p align="justify">
    Após isto, foram adicionados botões para implementar uma interação com mouse. A interação foi implemenada com dois testes de colisão: um teste esfera-caixa, em que a esfera dá uma distância de interação possível, e um teste raio-caixa. Estes dois testes juntos definem o comportamente se é possível interagir com um objeto, no caso, com um botão. Um modelo de tubarão foi adicionado simplesmente por ele ser suficientemente complexo. Por fim, foram adicionados os modelos de iluminação e as texturas. A luz da aplicação é única, saindo da câmera, para evitar dor de cabeça com a iluminação. Como todos os objetos possuem textura foi fácil aplicá-las graças as coordenadas de texutra. Com tudo isso pronto, foi possível trabalhar em algo mais elaborado. 
</p>

### Aplicação Final
<p align="justify">
    A aplicação final é semelhante a um genero de jogos de tubarão que consistem basicamente em controlar um tubarão e comer coisas. Foi adicionado uma movimentação para o tubarão, o tubarão possui uma movimentação de "tanque" (adendo: sem ré), rotacionando no próprio eixo (Y) e se movendo apenas para frente, na direção de um vetor calculado a partir de sua AABB. Além disso ele pode subir ou descer. O tubarão só pode se mover quando está com a câmera look-at que está sempre olhando para ele.
</p>
<p align="justify">
    A movimentação é feita atualizando a matriz de modelagem do tubarão: Translação x Matriz de modelagem atual x Rotação em Y. Está ordem preserva as alterações feitas anteriormente e rotaciona corretamente o tubarão no seu eixo.
</p>
<p align="justify">
    Foi construído um cenário de fundo do mar, adicionando peixes, algas, golfinhos e uma baleia. A baleia se movimenta seguindo uma curva de Bezier e só aparece quando o tubarão está suficientemente grande.
</p>
<p align="justify">
    A arena inicial foi descartada e suas antigas colisões passaram para o tubarão para o impedir de poder atravessar o chão, a resposta de colisão deste teste caixa-caixa é basicamente o up vector para anular a velocidade vertical. 
</p>
<p align="justify">
    Também foi passado para o tubarão a câmera livre, findando a ser a sua visão. Com isto, a interação com os botões passou a ser com os bichos do cenário. O antigo ato de apertar o botão virou a ação de comer um bicho. Cada bicho aumenta a capacidade tubarão: aumentando seu tamanho, sua velocidade e seu raio de interação (da esfera). Cada bicho aumenta a capacidade do tubarão conforme sua "nutritividade" (peixe > golfinho > baleia).   
</p>

## Inserir imagens aqui

## Manual
<p align="justify">
<strong>W</strong>: Faz o tubarão se movimentar na direção que está olhando.
</p>
<p align="justify">
<strong>A:</strong> Rotaciona o tubarão no eixo Y.
</p>
 <p align="justify">
<strong>D</strong>: Rotaciona o tubarão no eixo Y (direção oposta à rotação com A).
</p>
 <p align="justify">
<strong>SHIFT</strong>: Sobe o tubarão.
 </p>
<p align="justify">
<strong>CTRL</strong>: Desce o tubarão.
</p>
<p align="justify">
<strong>E</strong>: Entra no modo de câmera livre.
</p>
<p align="justify">
<strong>BOTÃO ESQUERDO DO MOUSE</strong>: Se estiver no modo de câmera livre, numa distância correta e olhando para outro bicho, come o bicho ao clicar.
</p>

## Compilação e execução:
<p align="justify">
    
### Linux:
Com o terminal na pasta raiz do projeto:
1. Executar o comando "Make" para compilar. 
2. Executar o comando "Make run" para executar.
</p>
<p align="justify">
O trabalho foi desenvolvido em cima dos códigos do laboratório e nenhuma outra biblioteca foi adicionada, então, possivelmente qualquer outro forma de compilar usada nos laboratórios deve funcionar.
</p>
