# HoshiOS

HoshiOS é um sistema operacional experimental para computadores x86 de 32 bits, desenvolvido principalmente em C e Assembly.

O projeto tem finalidade educacional e busca explorar conceitos de baixo nível como inicialização do processador, interrupções, comunicação com hardware, gerenciamento de memória, entrada de usuário e armazenamento em disco.

## Funcionalidades atuais

- Bootloader x86 adaptado para o HoshiOS em modo real de 16 bits
- Transição para modo protegido de 32 bits
- Kernel freestanding escrito em C
- Tabela GDT
- Tabela IDT com tratamento básico de exceções
- Controlador de interrupções PIC
- Temporizador PIT
- Saída de texto e cursor em modo VGA
- Teclado PS/2 por polling
- Mouse PS/2 por interrupção
- Shell interativo
- Detecção do mapa de memória pela BIOS usando E820
- Gerenciador de memória física com páginas de 4 KiB
- Bitmap do PMM dimensionado conforme a memória detectada
- Proteção das regiões reservadas, do kernel e do próprio bitmap
- Driver ATA PIO com endereçamento LBA28
- Leitura e escrita de setores
- Detecção da capacidade do disco com ATA IDENTIFY
- Tratamento de kernel panic
- Verificação do limite de tamanho do kernel durante a compilação
- Driver de comunicação serial COM1
- Logs de inicialização e kernel panic pela porta serial
- Teste de estresse do gerenciador de memória física
- Compatibilidade testada no QEMU e Oracle VirtualBox

## HoshiIK

O kernel interno do HoshiOS é chamado **HoshiIK**, abreviação de **Hoshi Internal Kernel**.

O HoshiIK constitui o núcleo do sistema e é responsável por inicialização, interrupções, gerenciamento de memória, tratamento de falhas e comunicação com os drivers.

O desenvolvimento de novas funcionalidades está temporariamente pausado. A versão atual permanece experimental e voltada para estudos de sistemas operacionais.

## Comandos disponíveis

| Comando | Descrição |
|---|---|
| `help` | Mostra os comandos disponíveis |
| `about` | Exibe informações sobre o HoshiOS |
| `clear` | Limpa a tela |
| `echo <texto>` | Exibe um texto |
| `reboot` | Reinicia o computador |
| `sysinfo` | Mostra informações do sistema |
| `ascii` | Exibe uma arte ASCII |
| `color <cor>` | Altera a cor do terminal |
| `meminfo` | Mostra o tamanho atual do kernel |
| `version` | Mostra a versão do HoshiOS |
| `atatest` | Testa o driver ATA PIO |
| `e820test` | Mostra as regiões de memória detectadas pela BIOS |
| `pmmtest` | Testa a alocação e liberação de páginas físicas |
| `pmmstress` | Aloca, verifica e libera 1024 páginas físicas |
| `serialtest` | Envia uma mensagem de teste pela porta serial COM1 |

## Estrutura do projeto

```text
HoshiOS/
├── arch/
│   └── x86/              # IDT, PIC, interrupções e operações de I/O
├── boot/
│   ├── bootloader.asm    # Bootloader e detecção de memória E820
│   └── kernel_entry.asm  # Entrada e preparação do kernel
├── drivers/
│   ├── input/            # Teclado e mouse PS/2
│   ├── serial/           # Comunicação e logs pela porta serial COM1
│   ├── storage/          # Driver ATA PIO
│   ├── timer/            # Temporizador PIT
│   └── video/            # Driver VGA
├── include/              # Tipos e cabeçalhos compartilhados
├── kernel/
│   ├── memory/           # Mapa E820 e gerenciador de memória física
│   ├── kernel.c          # Inicialização principal
│   ├── panic.c           # Tratamento de falhas fatais
│   └── shell.c           # Shell e comandos
├── third_party/          # Licenças de código de terceiros
├── linker.ld             # Script do linker
└── makefile              # Sistema de compilação
```

## Requisitos

O projeto foi desenvolvido para ser compilado em Linux ou WSL.

São necessários:

- GCC com suporte a 32 bits
- GNU Binutils
- NASM
- GNU Make
- QEMU para executar o sistema

Em distribuições baseadas em Debian ou Ubuntu:

```bash
sudo apt update
sudo apt install build-essential gcc-multilib binutils nasm make qemu-system-x86
```

## Compilação

Clone o repositório:

```bash
git clone https://github.com/Pedrodev75/HoshiOS.git
cd HoshiOS
```

Compile o sistema:

```bash
make
```

A imagem de disco será criada em:

```text
build/kernel.img
```

Para executar com uma quantidade diferente de memória:

```bash
make run QEMU_MEMORY=1024M

O processo de compilação também verifica se o kernel ultrapassou o limite suportado pelo bootloader.

## Executando no QEMU

Para compilar e iniciar o HoshiOS:

```bash
make run
```

Também é possível executar a imagem manualmente:

```bash
qemu-system-i386 -m 32M -drive format=raw,file=build/kernel.img
```

Para remover os arquivos gerados:

```bash
make clean
```

## Gerenciamento de memória

Durante o boot, o HoshiOS consulta a BIOS por meio da interface E820. O kernel utiliza o mapa recebido para liberar somente regiões classificadas como utilizáveis.

O gerenciador de memória física divide a RAM em páginas de 4 KiB e mantém um bitmap com o estado de cada página. Permanecem protegidos:

- o primeiro 1 MiB de memória;
- as regiões reservadas pela BIOS;
- a memória ocupada pelo kernel;
- o próprio bitmap do gerenciador.

O PMM oferece alocação e liberação de páginas físicas completas. A alocação de tamanhos menores e arbitrários será responsabilidade do futuro heap do kernel.

## Estado do projeto

O HoshiOS ainda está em desenvolvimento e não deve ser utilizado como sistema operacional para uso cotidiano ou armazenamento de dados importantes.

Atualmente, o projeto executa em modo kernel único. Já possui gerenciamento de páginas físicas, mas ainda não implementa heap, paginação, memória virtual, proteção entre processos, multitarefa ou sistema de arquivos completo.

## Próximos objetivos

- Implementar um heap do kernel com `kmalloc` e `kfree`
- Implementar paginação e memória virtual
- Desenvolver o HoshiFS
- Criar uma camada genérica para dispositivos de bloco
- Adicionar comandos para visualizar e manipular arquivos
- Melhorar o suporte ao teclado
- Adicionar uma biblioteca padrão básica para o kernel
- Criar testes automatizados
- Expandir o tratamento de exceções
- Melhorar a compatibilidade com hardware real

## Licença

O código original do HoshiOS é distribuído sob a **GNU General Public License versão 3 ou posterior**.

Consulte [LICENSE.md](LICENSE.md) para ler a licença completa.

O bootloader contém código derivado do projeto [Laurix](https://github.com/lowwryzen/laurix), criado por lowwryzen e disponibilizado sob a licença MIT.

As informações de atribuição e a licença correspondente estão disponíveis em:

- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
- [third_party/lowwryzen-MIT.md](third_party/lowwryzen-MIT.md)

## Autor

Desenvolvido por [Pedrodev75](https://github.com/Pedrodev75).