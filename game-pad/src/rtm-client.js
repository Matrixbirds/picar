import AgoraRTM from 'agora-rtm-sdk';
import EventEmitter from 'events';

export default class RTMClient extends EventEmitter {
  constructor() {
    super();
    this.channels = {};
    this._logined = false;
  }

  init(appId) {
    this.client = AgoraRTM.createInstance(appId);
    this.subscribeClientEvents();
  }

  // subscribe client events
  subscribeClientEvents() {
    const clientEvents = [
      "ConnectionStateChanged",
      "MessageFromPeer",
    ];
    clientEvents.forEach((eventName) => {
      this.client.on(eventName, (...args) => {
        console.log("emit ", eventName, ...args);
        // log event message
        this.emit(eventName, ...args);
      });
    });
  }

  // subscribe channel events
  subscribeChannelEvents(channelName) {
    const channelEvents = [
      "ChannelMessage",
      "MemberJoined",
      "MemberLeft"
    ];
    channelEvents.forEach((eventName) => {
      this.channels[channelName].channel.on(eventName, (...args) => {
        console.log("emit ", eventName, args);
        this.emit(eventName, {channelName, args: args});
      });
    });
  }

  async login(accountName, token) {
    this.accountName = accountName;
    return this.client.login({uid: this.accountName, token});
  }

  async logout() {
    return this.client.logout();
  }

  async joinChannel(name) {
    console.log("joinChannel", name)
    const channel = this.client.createChannel(name);
    this.channels[name] = {
      channel,
      joined: false // channel state
    }
    this.subscribeChannelEvents(name)
    return channel.join();
  }

  async leaveChannel(name) {
    console.log("leaveChannel", name);
    if (!this.channels[name] ||
      (this.channels[name]
        && !this.channels[name].joined)) return;
    return this.channels[name].channel.leave();
  }

  async sendChannelMessage(text, channelName) {
  console.log("sendChannelMessage", text, channelName);
    if (!this.channels[channelName] || !this.channels[channelName].joined) return;
    return this.channels[channelName].channel.sendMessage({text});
  }

  async sendPeerMessage(text) {
    const peerId = "agora.pi2";
    console.log("sendPeerMessage", text, peerId);
    return this.client.sendMessageToPeer({text}, peerId.toString());
  }

  async queryPeersOnlineStatus(memberId) {
    console.log("queryPeersOnlineStatus", memberId);
    return this.client.queryPeersOnlineStatus([memberId]);
  }
}