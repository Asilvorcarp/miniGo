# %%
import llvmlite.binding as llvm
import copy
from random import choice
from typing import Optional, Union, Any, Set, List, Tuple, Dict, Collection
from llvmlite.binding import ValueRef
import matplotlib.pyplot as plt
import networkx as nx
import argparse

# TODO:
# 1. support global
# 2. why is there diff in matrix 1.in

class Graph:
    def __init__(self):
        self._adjacency_list = {}

    def __copy__(self):
        cls = self.__class__
        new_graph = self.__new__(cls)
        new_graph._adjacency_list = copy.deepcopy(self._adjacency_list)
        return new_graph

    def add_edge(self, x, y):
        # Add y to x
        x_list = self._adjacency_list.get(x, [])
        if y not in x_list:
            x_list.append(y)
        self._adjacency_list[x] = x_list

        # Add x to y
        y_list = self._adjacency_list.get(y, [])
        if x not in y_list:
            y_list.append(x)
        self._adjacency_list[y] = y_list

    def contains_edge(self, x, y):
        return y in self._adjacency_list.get(x, [])

    def remove_node(self, node):
        if node in self._adjacency_list:
            self._adjacency_list.pop(node)
        for key in self._adjacency_list.keys():
            if node in self._adjacency_list.get(key):
                self._adjacency_list.get(key).remove(node)

    def rename_node(self, from_label, to_label):
        from_list = self._adjacency_list.pop(from_label, [])
        to_list = self._adjacency_list.get(to_label, [])
        self._adjacency_list[to_label] = list(set(from_list + to_list))

        for key in self._adjacency_list.keys():
            self._adjacency_list[key] = list(set(
                [to_label if value ==
                    from_label else value for value in self._adjacency_list[key]]
            ))

    def neighbors(self, x):
        return self._adjacency_list.get(x, [])

    def plot(self, coloring, title):
        G = nx.Graph()

        # Sorting to get repeatable graphs
        nodes = sorted(self._adjacency_list.keys())
        ordered_coloring = [coloring.get(node, 'grey') for node in nodes]
        G.add_nodes_from(nodes)

        for key in self._adjacency_list.keys():
            for value in self._adjacency_list[key]:
                G.add_edge(key, value)

        plt.title(title)
        plt.size = (20, 20)
        # other layout available at https://networkx.org/documentation/stable/reference/drawing.html
        nx.draw(G, pos=nx.shell_layout(G), node_color=ordered_coloring,
                with_labels=True)
        plt.show()

    def all_nodes(self):
        return self._adjacency_list.keys()


def color_graph(g: Graph, regs: Collection[str], colors: List[str]) -> Optional[Dict[str, str]]:
    if len(regs) == 0:
        return {}

    node = next((node for node in regs if len(
        g.neighbors(node)) < len(colors)), None)
    if node is None:
        return None

    g_copy = copy.copy(g)
    g_copy.remove_node(node)
    coloring = color_graph(g_copy, [n for n in regs if n != node], colors)
    if coloring is None:
        return None

    neighbor_colors = [coloring[neighbor] for neighbor in g.neighbors(node)]
    coloring[node] = [
        color for color in colors if color not in neighbor_colors][0]

    return coloring


def save() -> List[str]:
    return [f"pushq %r{x}" for x in range(8, 16)]


def restore() -> List[str]:
    return [f"popq %r{x}" for x in range(8, 16)][::-1]


def codeGenForFunc(fn: ValueRef, showImg=True) -> List[str]:
    funcName = fn.name
    print("codeGenForFunc", funcName)

    ins: List[ValueRef] = []
    allocas: Dict[str, int] = {}
    useDef: Dict[int, List[Any]] = {}
    labelToIdx: Dict[str, int] = {}

    for b in fn.blocks:
        labelToIdx[b.name] = len(
            [_ for _ in ins if _.opcode not in ["alloca"]])  # TODO ensure this
        ins += [_ for _ in b.instructions]
    # if '' in labelToIdx.keys():
    labelToIdx.pop("")

    print(labelToIdx)

    idxToLabel = {value: key for key, value in labelToIdx.items()}

    # collect allocas and construct use-def chain
    idx = 0
    for i in ins:
        op = i.opcode
        if op == "alloca":
            # TODO spill also increase size
            # TODO not actually alloca for arg4+
            if str(i.type) == "i32*":
                # allocas[i.name] = 4
                allocas[i.name] = 8
            else:
                allocas[i.name] = 8
            idx -= 1
        elif op == "store":
            use = []
            for j in i.operands:
                if j.name != "" and j.name not in allocas:
                    use.append(j.name)
            useDef[idx] = [i, set(use), set()]
        elif op == "br":
            use = []
            for j in i.operands:
                if str(j.type) != "label":
                    if j.name != "" and j.name not in allocas:
                        use.append(j.name)
            useDef[idx] = [i, set(use), set()]
        elif op == "call":
            use = []
            defs = []
            if i.name != "":
                defs.append(i.name)
            for j in i.operands:
                isFnType = '(' in str(j.type) and ')' in str(j.type)
                if not isFnType and j.name != "" and j.name not in allocas:
                    use.append(j.name)
            useDef[idx] = [i, set(use), set(defs)]
        else:
            use = []
            defs = []
            if i.name != "":
                defs.append(i.name)
            for j in i.operands:
                if j.name != "" and j.name not in allocas:
                    use.append(j.name)
            useDef[idx] = [i, set(use), set(defs)]
        idx += 1

    # get succ
    for idx in useDef:
        if idx == len(useDef)-1:
            useDef[idx].insert(1, [])
            continue
        theI = useDef[idx][0]
        op = theI.opcode
        if op == "br":
            print(theI)
            labels: List[ValueRef] = [
                _ for _ in theI.operands if str(_.type) == "label"]
            useDef[idx].insert(1, [labelToIdx[_.name] for _ in labels])
        else:
            useDef[idx].insert(1, [idx+1])

    # print useDef
    for i in useDef:
        print(i, useDef[i][1:])

    # get out in
    theIn = [set() for _ in useDef]
    theOut = [set() for _ in useDef]
    theIn2 = [set() for _ in useDef]
    theOut2 = [set() for _ in useDef]

    init = 1

    def pretty(s: Set[str]):
        return "{" + ", ".join(s) + "}"

    def done():
        nonlocal init
        if init:
            init = 0
            return False
        for i in range(len(theIn)):
            if theIn[i] != theIn2[i] or theOut[i] != theOut2[i]:
                return False
        print("done")
        return True

    count = 0
    while not done():
        # for ff in range(9):
        count += 1
        theIn2 = copy.deepcopy(theIn)
        theOut2 = copy.deepcopy(theOut)
        # reverse
        for idx in range(len(useDef)-1, -1, -1):
            theIn[idx] = useDef[idx][2] | (theOut[idx] - useDef[idx][3])
            theOut[idx] = set()
            for succ in useDef[idx][1]:
                theOut[idx] |= theIn[succ]
    print("count", count)

    # print def use in out
    for i in range(len(theIn)):
        print(i, pretty(useDef[i][3]), pretty(useDef[i][2]),
              "|", pretty(theIn[i]), pretty(theOut[i]))

    print(allocas)
    local = 8 * len(allocas)
    print(local)

    allTemp = set.union(*([useDef[idx][2]
                        for idx in useDef] + [useDef[idx][3] for idx in useDef]))
    print(allTemp)

    def build_graph() -> Graph:
        g = Graph()
        # for t in allTemp:
        #     print("add", t)
        #     g.add_edge(t, t)
        for idx in useDef:
            outs = theOut[idx]
            defs = useDef[idx][3]
            for aDef in defs:
                for aOut in outs:
                    if aDef != aOut:
                        print("add", aDef, aOut)
                        g.add_edge(aDef, aOut)
        return g

    g = build_graph()
    colors = [i for i in range(8)]
    coloring = color_graph(g, list(allTemp), colors)
    print("coloring", coloring)
    print("single nodes:", set(allTemp) - set(g.all_nodes()))
    print("color needed:", len(set(coloring.values())))
    if showImg:
        g.plot(coloring, f"{funcName}\nInterference Graph")
    # get depth in the stack
    depth: Dict[str, int] = {}
    for k, v in allocas.items():
        sum = 0
        for k2, v2 in allocas.items():
            sum += v2
            if k == k2:
                break
        depth[k] = sum
    print("depth", depth)

    # to real register
    def toR(reg: str | ValueRef, deRefer=False) -> str:
        if reg.name != '':
            reg = reg.name
        else:
            reg = str(reg)
        # now: "i32 0" / "t99"
        print("toR", reg)  # debug
        if 'i32' in reg or 'i64' in reg:
            _, n = reg.split(' ')
            if n=='null':
                return '$0'
            return f"${n}"
        elif '.arg' in reg:
            X, n = reg.split('.arg')
            n = int(n)
            if n == 0:
                return "%rdi"
            elif n == 1:
                return "%rsi"
            elif n == 2:
                return "%rdx"
            elif n == 3:
                return "%rcx"
            elif n >= 4:
                # update depth
                # ! assert X.argN => (X)
                # ra, rbp at 0, 8
                depth[X] = -((n-4)*8+16)
                return f'{-depth[X]}(%rbp)'
        elif deRefer and 'local_' in reg:
            return f'{-depth[reg]}(%rbp)'
        if deRefer:
            return f'(%r{coloring[reg]+8})'
        return f"%r{coloring[reg]+8}"

    asm: List[str] = []
    asm += [f"\t.globl {funcName}"]
    asm += [f"\t{funcName}:"]
    asm += [f"pushq %rbp"]
    asm += [f"movq %rsp, %rbp"]
    asm += [f"subq ${local}, %rsp"]
    asm += save()
    for idx in useDef:
        i: ValueRef = useDef[idx][0]
        print(">", i)
        op = i.opcode
        regs = [_ for _ in i.operands]
        cmds = []
        # insert label
        label = idxToLabel.get(idx, None)
        if label != None:
            asm += [f'\t{label}:']
        if op == "add":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            # TODO check for other cases!
            # do not overwrite rs2
            if rd == rs2:
                rs1, rs2 = rs2, rs1
            cmd1 = f"movq {rs1}, {rd}"
            cmd2 = f"addq {rs2}, {rd}"
            cmds = [cmd1, cmd2]
        elif op == "sub":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            toProtect = []
            insert = []
            if rd == rs2:
                insert = [f"movq {rs2}, %rcx"]
                rs2 = "%rcx"
                toProtect += ['%rcx']
            cmds += [f"pushq {r}" for r in toProtect]
            cmds += insert
            cmds += [f"movq {rs1}, {rd}"]
            cmds += [f"subq {rs2}, {rd}"]
            cmds += [f"popq {r}" for r in toProtect][::-1]
        elif op == "or":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            # do not overwrite rs2
            if rd == rs2:
                rs1, rs2 = rs2, rs1
            cmd1 = f"movq {rs1}, {rd}"
            cmd2 = f"orq {rs2}, {rd}"
            cmds = [cmd1, cmd2]
        elif op == "and":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            # do not overwrite rs2
            if rd == rs2:
                rs1, rs2 = rs2, rs1
            cmd1 = f"movq {rs1}, {rd}"
            cmd2 = f"andq {rs2}, {rd}"
            cmds = [cmd1, cmd2]
        elif op == "mul":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            insert = []
            toProtect = ['%rax', '%rdx']
            # need to allocate a reg for const
            if '$' in rs2:
                insert = [f"movq {rs2}, %rcx"]
                rs2 = "%rcx"
                toProtect += ['%rcx']
            cmds += [f"pushq {r}" for r in toProtect]
            cmds += [f"movq {rs1}, %rax"]
            cmds += insert
            cmds += [f"imulq {rs2}"]
            cmds += [f"movq %rax, {rd}"]
            cmds += [f"popq {r}" for r in toProtect][::-1]
        elif op == "sdiv":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            insert = []
            toProtect = ['%rax', '%rdx']
            # need to allocate a reg for const
            if '$' in rs2:
                insert = [f"movq {rs2}, %rcx"]
                rs2 = "%rcx"
                toProtect += ['%rcx']
            cmds += [f"pushq {r}" for r in toProtect]
            cmds += [f"movq {rs1}, %rax"]
            cmds += [f"cqto"]
            cmds += insert
            cmds += [f"idivq {rs2}"]
            cmds += [f"movq %rax, {rd}"]
            cmds += [f"popq {r}" for r in toProtect][::-1]
        elif op == "srem":
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            insert = []
            toProtect = ['%rax', '%rdx']
            # need to allocate a reg for const
            if '$' in rs2:
                insert = [f"movq {rs2}, %rcx"]
                rs2 = "%rcx"
                toProtect += ['%rcx']
            cmds += [f"pushq {r}" for r in toProtect]
            cmds += [f"movq {rs1}, %rax"]
            cmds += [f"cqto"]
            cmds += insert
            cmds += [f"idivq {rs2}"]
            cmds += [f"movq %rdx, {rd}"]
            cmds += [f"popq {r}" for r in toProtect][::-1]
        elif op == 'store':
            rs = toR(regs[0])
            rd = toR(regs[1], True)
            if '(%rbp)' in rs:
                # arg4+, in stack, no need to store
                cmds = []
            else:
                cmds = [f"movq {rs}, {rd}"]
        elif op == 'load':
            rd = toR(i)
            rs = toR(regs[0], True)
            cmd = f'movq {rs}, {rd}'
            cmds = [cmd]
        elif op == 'icmp':
            # slt, ...
            exactType = str(i).split('icmp ')[1].split()[0]
            # ! assert first label is always next
            toSet = {
                'eq': 'sete',
                'ne': 'setne',
                'slt': 'setl',
                'sle': 'setle',
                'sgt': 'setg',
                'sge': 'setge',
            }
            a = toR(regs[0])
            b = toR(regs[1])
            rd = toR(i)
            # this is "a-b"
            cmds += [f"pushq %rax"]
            cmds += [f"movq $0, %rax"]
            cmds += [f"cmpq {b}, {a}"]
            cmds += [f"{toSet[exactType]} %al"]
            cmds += [f"movq %rax, {rd}"]
            cmds += [f"popq %rax"]
        elif op == 'br':
            labels = []
            for j in regs:
                if str(j.type) == "label":
                    labels += [j.name]
            labels.reverse()  # the right order
            if len(labels) == 1 and labelToIdx.get(labels[0]) != idx+1:
                cmds = [f"jmp {labels[0]}"]
            elif len(labels) == 2:
                # asm += [f"\t{str(labels)}"]
                rs = toR(regs[0])
                cmds += [f"cmpq $0, {rs}"]
                cmds += [f"je {labels[1]}"]
        elif op == 'call':
            rd = toR(i) if i.name != '' else None
            fn = regs[-1].name + "@PLT"
            args = [toR(_) for _ in regs[:-1]]
            theMap = {
                0: "%rdi",
                1: "%rsi",
                2: "%rdx",
                3: "%rcx",
            }
            # args
            reversePush = []
            for x, arg in enumerate(args):
                if x < 4:
                    cmds += [f"movq {arg}, {theMap[x]}"]
                else:
                    reversePush += [f"pushq {arg}"]
            cmds += reversePush[::-1]
            cmds += [f"call {fn}"]
            if rd != None:
                cmds += [f"movq %rax, {rd}"]
        elif op == 'ret':
            if len(regs) != 0:
                res = toR(regs[0])
                cmds += [f"movq {res}, %rax"]
            cmds += [f"jmp {funcName}_end"]
        elif op == 'getelementptr':
            rs1 = toR(regs[0])
            rs2 = toR(regs[1])
            rd = toR(i)
            insert = []
            toProtect = []
            if '$' in rs2:
                # alloca reg for const
                insert += [f"movq {rs2}, %rax"]
                rs2 = "%rax"
                toProtect += ['%rax']
            cmds += [f"pushq {r}" for r in toProtect]
            cmds += insert
            cmds += [f"leaq ({rs1},{rs2},{8}), {rd}"]
            cmds += [f"popq {r}" for r in toProtect][::-1]
        elif op == 'bitcast':
            rs = toR(regs[0])
            rd = toR(i)
            cmd = f"movq {rs}, {rd}"
            cmds = [cmd]
        else:
            cmds = ["!!!TODO!!!"]
        asm += [f'  #{str(i)}']
        asm += cmds
        print("  -", cmds)

    asm += [f"\t{funcName}_end:"]
    asm += restore()
    asm += [f"movq %rbp, %rsp"]
    asm += [f"popq %rbp"]
    asm += [f"retq"]

    print("-- ASM: --")
    print("\n".join(asm))

    return asm


def codeGen(fileName: str, showImg=True) -> List[List[str]]:
    llvm.initialize()
    ir = open(fileName).read()
    # the ModuleRef
    mod = llvm.parse_assembly(ir)
    mod.verify()
    print(mod.triple)
    asms = []
    for fn in mod.functions:
        if fn.is_declaration:
            continue
        asm = codeGenForFunc(fn, showImg)
        asms.append(asm)
    # llvm.shutdown()
    return asms


def ll2asm(inFile, outFile, showImg=True):
    asms = codeGen(inFile, showImg)
    flat = '\t.text\n'
    flat += '\n'.join(['\n'.join(x + ['']) for x in asms])
    with open(outFile, 'w') as f:
        f.write(flat)

# %%

if __name__ == "__main__":

    parser = argparse.ArgumentParser(description="The Backend of the Compiler")

    parser.add_argument('-f', '--file', type=str, help="Input file name")
    parser.add_argument('-o', '--output', type=str, help="Output file name")

    args = parser.parse_args()

    input_file = args.file
    output_file = args.output

    ll2asm(input_file, output_file, showImg=False)

# %%



