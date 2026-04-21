# Kita

## 快速开始

### 环境要求

- Windows 10/11
- Git
- Visual Studio 2022/2026（包含 C++ 桌面开发组件）

### 克隆项目（包含子模块）

```powershell
git clone --recurse-submodules https://github.com/NiKuliCat/Kita.git
cd Kita
```

如果仓库已经克隆但没有拉取子模块，请执行：

```powershell
git submodule update --init --recursive
```

### 生成工程文件

```powershell
.\Setup.bat
```

默认生成 VS2026 工程。也可以显式指定版本：

```powershell
.\Setup.bat 2022
.\Setup.bat 2026
```

等价写法：

```powershell
.\Setup.bat vs2022
.\Setup.bat vs2026
```

执行后会生成 `Kita.slnx` 解决方案。

### 构建（Debug | x64）

```powershell
& "G:\IDE\VS2026\application\MSBuild\Current\Bin\MSBuild.exe" ".\Kita.slnx" /m /p:Configuration=Debug /p:Platform=x64
```
