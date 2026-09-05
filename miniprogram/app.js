App({
  globalData: {
    apiBase: "https://your-backend.example.com",
    deviceId: "desktop-agent-001",
    refreshIntervalMs: 1500,
    controlToken: ""
  },
  onLaunch() {
    const apiBase = wx.getStorageSync("apiBase");
    if (apiBase) {
      this.globalData.apiBase = apiBase;
    }
    wx.removeStorageSync("controlToken");
    this.globalData.controlToken = "";
  }
});
