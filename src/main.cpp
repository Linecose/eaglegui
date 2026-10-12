name: Build EagleGUI Native Module

on:
  push:
    branches: [ main, master ]
  pull_request:
    branches: [ main, master ]

jobs:
  build-windows:
    runs-on: windows-latest

    steps:
    - name: Shkarko kodin (Checkout)
      uses: actions/checkout@v4

    - name: Shto Node.js
      uses: actions/setup-node@v4
      with:
        node-version: '20'

    - name: Instalo Qt
      uses: jurplel/install-qt-action@v3
      with:
        version: '6.5.0'
        target: 'desktop'

    - name: Instalo varësitë e Node.js
      run: npm install

    - name: Kompilo me cmake-js
      run: npx cmake-js compile --release

    - name: Ruaj skedarin binar (.node)
      uses: actions/upload-artifact@v4
      with:
        name: eaglegui-binary
        path: build/Release/*.node
