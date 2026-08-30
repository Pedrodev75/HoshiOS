# HoshiOS

HoshiOS é um sistema operacional experimental para computadores x86 de 32 bits, desenvolvido principalmente em C e Assembly.

O projeto tem finalidade educacional e busca explorar conceitos de baixo nível como inicialização do processador, interrupções, comunicação com hardware, entrada de usuário e armazenamento em disco.

## Funcionalidades atuais

- Bootloader x86 adaptado para o HoshiOS em modo real de 16 bits
- Transição para modo protegido de 32 bits
- Kernel freestanding escrito em C
- Tabela GDT
- Tabela IDT com tratamento básico de exceções
- Controlador de interrupções PIC
- Temporizador PIT
- Saída de texto em modo VGA
- Cursor de hardware VGA
- Teclado PS/2 por polling
- Mouse PS/2 por interrupção
- Shell interativo
- Driver ATA PIO com endereçamento LBA28
- Leitura e escrita de setores
- Detecção da capacidade do disco com ATA IDENTIFY
- Tratamento de kernel panic
- Verificação do limite de tamanho do kernel durante a compilação

## Comandos disponíveis

O shell do HoshiOS atualmente oferece os seguintes comandos:

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

## Estrutura do projeto

```text
HoshiOS/
├── arch/
│   └── x86/              # IDT, PIC, interrupções e operações de I/O
├── boot/
│   ├── bootloader.asm    # Bootloader de 16 bits
│   └── kernel_entry.asm  # Entrada e preparação do kernel
├── drivers/
│   ├── input/            # Teclado e mouse PS/2
│   ├── storage/          # Driver ATA PIO
│   ├── timer/            # Temporizador PIT
│   └── video/            # Driver VGA
├── include/              # Tipos e cabeçalhos compartilhados
├── kernel/               # Kernel, shell e tratamento de falhas
├── third_party/          # Licenças de código de terceiros
├── linker.ld             # Script do linker
└── makefile              # Sistema de compilação
```

## Requisitos

O projeto foi desenvolvido para ser compilado em um ambiente Linux ou WSL.

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

O processo de compilação também verifica se o kernel ultrapassou o limite suportado pelo bootloader.

## Executando no QEMU

Para compilar e iniciar o HoshiOS:

```bash
make run
```

Também é possível executar a imagem manualmente:

```bash
qemu-system-i386 -drive format=raw,file=build/kernel.img
```

Para remover os arquivos gerados:

```bash
make clean
```

## Estado do projeto

O HoshiOS ainda está em desenvolvimento e não deve ser utilizado como sistema operacional para uso cotidiano ou armazenamento de dados importantes.

Atualmente, o projeto executa em modo kernel único, sem proteção entre processos, multitarefa, memória virtual ou sistema de arquivos completo.

## Próximos objetivos

- Desenvolver o HoshiFS
- Criar uma camada genérica para dispositivos de bloco
- Adicionar comandos para visualizar e manipular arquivos
- Implementar gerenciamento de memória
- Melhorar o suporte ao teclado
- Adicionar uma biblioteca padrão básica para o kernel
- Criar testes automatizados
- Expandir o tratamento de exceções
- Melhorar a compatibilidade com hardware real

## Licença

O código original do HoshiOS é distribuído sob a licença **GNU General Public License versão 3 ou posterior**.

Consulte [LICENSE.md](LICENSE.md) para ler a licença completa.

O bootloader contém código derivado do projeto [Laurix](https://github.com/lowwryzen/laurix), criado por lowwryzen e disponibilizado sob a licença MIT.

As informações de atribuição e a licença correspondente estão disponíveis em:

- [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)
- [third_party/lowwryzen-MIT.md](third_party/lowwryzen-MIT.md)

## Autor

Desenvolvido por [Pedrodev75](https://github.com/Pedrodev75).