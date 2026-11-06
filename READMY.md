# Guia prático de ROS 2 com C++ (Humble + WSL + Docker)

Este guia acompanha a apresentação **ROS 2 – Robot Operating System 2** (NorthSpace) e reúne, por ordem, todos os comandos e o código necessários para:

1. Preparar um ambiente ROS 2 Humble com WSL e Docker
2. Criar um pacote em C++
3. Publicar mensagens num tópico
4. Verificar a comunicação com `ros2 topic echo`

> **Legenda:** cada bloco de comandos indica onde deve ser executado:
> - 🐧 **WSL** → terminal do Ubuntu/WSL
> - 🐳 **Contentor** → terminal dentro do contentor Docker `ros2_humble`

---

## 1. Conceitos essenciais

| Conceito | Descrição |
|---|---|
| **Package** | Unidade de organização que agrupa código, nós, mensagens e outros recursos |
| **Nó (Node)** | Processo individual que executa uma função do robô (ex.: controlar motores, processar dados) |
| **Tópico (Topic)** | Canal de comunicação entre nós, baseado em publicação/subscrição (*publish/subscribe*) |
| **Mensagem (Message)** | Dados estruturados com tipo definido que circulam pelos tópicos (ex.: `std_msgs/msg/String`, `sensor_msgs/LaserScan`) |
| **Publisher** | Quem publica mensagens num tópico |
| **Subscriber** | Quem recebe mensagens de um tópico |

**Estrutura de um workspace:**

```
Workspace
├── Package 1
│   ├── Nodes (Node 1, Node 2)
│   ├── Messages
│   ├── Services
│   └── Actions
└── Package 2
    └── Nodes (Node 3)
```

**Fluxo deste exemplo:** o nó `publisher` publica uma mensagem de texto em `/chatter` a cada segundo; o comando `ros2 topic echo` funciona como subscritor e mostra as mensagens recebidas.

---

## 2. Pré-requisitos

- [ ] Ubuntu no WSL instalado e aberto
- [ ] Docker instalado e em execução, acessível a partir do WSL
- [ ] Ligação à Internet (para descarregar a imagem `ros:humble`)
- [ ] Visual Studio Code instalado, com o comando `code` disponível no Ubuntu/WSL

---

## 3. Preparação do ambiente

### 3.1 Confirmar o Docker e criar a pasta partilhada

🐧 **WSL**
```bash
docker --version
sudo docker info
mkdir -p ~/ros2_ws
```

Se `docker info` falhar, inicie o Docker antes de continuar.

### 3.2 Descarregar a imagem ROS 2 Humble

🐧 **WSL**
```bash
sudo docker pull ros:humble
```

### 3.3 Permitir usar o Docker sem `sudo`

🐧 **WSL**
```bash
sudo usermod -aG docker $USER
```

> ⚠️ Depois deste comando, **feche a sessão do Ubuntu/WSL e volte a entrar** para a alteração ter efeito. Até lá, continue a usar `sudo docker`.

### 3.4 Criar e abrir o contentor

🐧 **WSL**
```bash
sudo docker run -it --name ros2_humble -v ~/ros2_ws:/root/ros2_ws ros:humble
```

O volume (`-v`) faz com que `~/ros2_ws` no WSL corresponda a `/root/ros2_ws` no contentor. A partir daqui, o terminal está **dentro do contentor**.

### 3.5 Carregar o ambiente ROS 2 e verificar a instalação

🐳 **Contentor**
```bash
source /opt/ros/humble/setup.bash
ros2 -h
```

Se aparecer a ajuda do ROS 2, a instalação está correta.

---

## 4. Criar o workspace e o pacote

### 4.1 Criar a estrutura do workspace

🐳 **Contentor**
```bash
mkdir -p ~/ros2_ws/src
cd ~/ros2_ws/src
```

### 4.2 Criar o pacote C++

🐳 **Contentor**
```bash
ros2 pkg create meu_projeto_cpp --build-type ament_cmake --dependencies rclcpp std_msgs
```

Isto cria o pacote `meu_projeto_cpp`, configura o `ament_cmake` e regista as dependências `rclcpp` e `std_msgs`.

Estrutura criada:

```
ros2_ws/src/meu_projeto_cpp
├── include/meu_projeto_cpp
├── src
├── CMakeLists.txt
└── package.xml
```

### 4.3 Abrir o workspace no VS Code

🐧 **WSL** (noutro terminal)
```bash
cd ~/ros2_ws/src
code .
```

No VS Code, crie o ficheiro **`meu_projeto_cpp/src/main.cpp`**.

---

## 5. Código do publisher

Cole o seguinte código em `meu_projeto_cpp/src/main.cpp`. O nó cria um publisher para o tópico `/chatter` e usa um *timer* para publicar uma mensagem a cada segundo.

```cpp
#include <chrono>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;

class PublisherNode : public rclcpp::Node {
public:
  PublisherNode() : Node("publisher"), count_(0) {
    publisher_ = this->create_publisher<std_msgs::msg::String>("chatter", 10);
    timer_ = this->create_wall_timer(
      1s, std::bind(&PublisherNode::publish_message, this));
  }

private:
  void publish_message() {
    auto message = std_msgs::msg::String();
    message.data = "Olá do ROS 2! Mensagem #" +
                   std::to_string(count_++);
    RCLCPP_INFO(this->get_logger(), "A publicar: '%s'",
                message.data.c_str());
    publisher_->publish(message);
  }

  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  std::size_t count_;
};

int main(int argc, char * argv[]) {
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PublisherNode>());
  rclcpp::shutdown();
  return 0;
}
```

**O que cada parte faz:**

| Parte | Função |
|---|---|
| `rclcpp::init` / `shutdown` | Inicia e termina o ROS 2 |
| `class PublisherNode : public rclcpp::Node` | Define o nó (chama-se `publisher`) |
| `create_publisher<std_msgs::msg::String>("chatter", 10)` | Cria o publisher no tópico `/chatter` (fila de 10 mensagens) |
| `create_wall_timer(1s, ...)` | Chama `publish_message` a cada segundo |
| `publisher_->publish(message)` | Envia a mensagem para o tópico |
| `rclcpp::spin(...)` | Mantém o nó ativo |

> **Nota:** o executável chamar-se-á `publisher`, o nó chamar-se-á `publisher` e o tópico será `/chatter`.

---

## 6. Configurar o `CMakeLists.txt`

O `CMakeLists.txt` diz ao ROS 2 **o que compilar**; neste caso, um executável. Abra `meu_projeto_cpp/CMakeLists.txt` e deixe-o assim:

```cmake
cmake_minimum_required(VERSION 3.8)
project(meu_projeto_cpp)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

add_executable(publisher src/main.cpp)
ament_target_dependencies(publisher rclcpp std_msgs)

install(
  TARGETS publisher
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
```

**As 3 linhas-chave que tem de acrescentar** (se partir do ficheiro gerado por defeito):

```cmake
find_package(std_msgs REQUIRED)

add_executable(publisher src/main.cpp)
ament_target_dependencies(publisher rclcpp std_msgs)

install(TARGETS publisher DESTINATION lib/${PROJECT_NAME})
```

> **`package.xml`:** como o pacote foi criado com `--dependencies rclcpp std_msgs`, as dependências já devem estar declaradas. Se não estiverem, adicione:
> ```xml
> <depend>rclcpp</depend>
> <depend>std_msgs</depend>
> ```

---

## 7. Compilar e executar

🐳 **Contentor**
```bash
cd ~/ros2_ws
source /opt/ros/humble/setup.bash
colcon build
source install/setup.bash
ros2 run meu_projeto_cpp publisher
```

O publisher fica a correr e publica uma mensagem por segundo. Para parar: `Ctrl+C`.

Saída esperada:

```
[INFO] [...] [publisher]: A publicar: 'Olá do ROS 2! Mensagem #0'
[INFO] [...] [publisher]: A publicar: 'Olá do ROS 2! Mensagem #1'
```

### Depois de alterar o código

Repita sempre estes passos:

🐳 **Contentor**
```bash
cd ~/ros2_ws
colcon build
source install/setup.bash
ros2 run meu_projeto_cpp publisher
```

---

## 8. Verificar com `ros2 topic echo`

Com o publisher a correr no primeiro terminal, abra um **segundo terminal do Ubuntu/WSL** e entre no mesmo contentor:

🐧 **WSL**
```bash
docker exec -it ros2_humble bash
```

Agora, dentro do contentor (segundo terminal):

🐳 **Contentor**
```bash
source /opt/ros/humble/setup.bash
cd ~/ros2_ws
source install/setup.bash
ros2 topic echo /chatter
```

Deverá ver:

```
data: Olá do ROS 2! Mensagem #0
---
data: Olá do ROS 2! Mensagem #1
---
```

Para terminar: `Ctrl+C`.

### Comandos úteis de inspeção

🐳 **Contentor**
```bash
ros2 node list          # lista os nós ativos
ros2 topic list         # lista os tópicos
ros2 topic info /chatter  # tipo e nº de publishers/subscribers
```

---

## 9. Reabrir o ambiente

Se o contentor tiver sido parado, **não** volte a usar `docker run` (tentaria criar outro contentor com o mesmo nome).

🐧 **WSL**
```bash
docker start -ai ros2_humble
```

Já dentro do contentor:

🐳 **Contentor**
```bash
source /opt/ros/humble/setup.bash
cd ~/ros2_ws
source install/setup.bash
```

Se o contentor já estiver em execução, abra outro terminal nele com:

🐧 **WSL**
```bash
docker exec -it ros2_humble bash
```

---

## 10. Problemas comuns

| Situação | Verificação ou solução |
|---|---|
| Docker não está acessível | Confirme que está instalado e em execução: `docker info` ou `sudo docker info` |
| Permissão negada ao usar Docker | Termine a sessão e volte a entrar depois de `sudo usermod -aG docker $USER`, ou use `sudo docker` |
| O nome `ros2_humble` já existe | Use `docker start -ai ros2_humble` em vez de `docker run` |
| `ros2` ou `colcon` não encontrado | Dentro do contentor: `source /opt/ros/humble/setup.bash` |
| Package ou executável não encontrado | Esteja em `~/ros2_ws`, execute `colcon build` e depois `source install/setup.bash` |
| O tópico não mostra mensagens | Confirme que o publisher continua a correr e que o 2.º terminal fez `source` dos **dois** ambientes antes de `ros2 topic echo /chatter` |
| Alterações não aparecem | Volte a compilar, `source install/setup.bash` e reinicie o publisher |
| Erro de permissões nos ficheiros do workspace (criados pelo contentor como root) | No WSL: `sudo chown -R $USER:$USER ~/ros2_ws` |

---

## 11. Correções face aos slides

Alguns comandos dos slides têm pequenos erros. Use as versões deste guia:

| Slide | No slide | Correto |
|---|---|---|
| 5 | `source /opt/ros/humble/setup.sh` | `source /opt/ros/humble/setup.bash` |
| 6 | `mkdir -p root/ros2_ws/src && cd ~/ros2_ws/src` | `mkdir -p ~/ros2_ws/src && cd ~/ros2_ws/src` |
| 6 | `--dependencies rclcpp` | `--dependencies rclcpp std_msgs` (o publisher usa `std_msgs`) |
| 6 | `cd root/ros2_ws && colcon build` | `cd ~/ros2_ws && colcon build` |
| 9 | `sourece insatll /setup.sh` | `source install/setup.bash` |
| 7 | Publica **uma só vez** em `/ola` e termina | Este guia usa um *timer* e publica continuamente em `/chatter`, o que permite ver a comunicação com `ros2 topic echo` |

---

## 12. Resumo final

- [ ] Imagem ROS 2 Humble descarregada e contentor disponível
- [ ] Workspace criado em `~/ros2_ws`
- [ ] Pacote C++ criado com `rclcpp` e `std_msgs`
- [ ] Publisher compilado e executado com `ros2 run meu_projeto_cpp publisher`
- [ ] Mensagens verificadas no tópico `/chatter`

**Sequência essencial:**
preparar o contentor → criar o pacote → escrever o nó → configurar o CMake → `colcon build` → `source install/setup.bash` → iniciar o publisher → observar `/chatter`.

---

### Próximo passo sugerido

Criar um **subscriber** em C++ (um segundo nó que subscreve `/chatter` com `create_subscription`) para substituir o `ros2 topic echo` e completar o ciclo publish/subscribe.
