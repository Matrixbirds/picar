import React, {useState, useEffect, useRef} from 'react';
import { toast } from 'react-toastify';
import RTMClient from './rtm-client';
const APP_ID = process.env.REACT_APP_AGORA_APP_ID;


const MAX_SPEED = 120;
const MIN_SPEED = 50;

const rtm = new RTMClient();

export default function GamePad () {

  const [connectState, setRtmConnectState] = useState('DISCONNECTED');
  const [login, setLogin] = useState(false);
  const [touch, setTouch] = useState(false);
  const [speedTouch, setSpeedTouch] = useState(false);

  const [start, setStart] = useState(null);

  const [connect, setConnect] = useState('false');

  const [battery, setBattery] = useState(100);

  const [speed, setSpeed] = useState(60);

  const [direct, setDirect] = useState(null);

  const [logs, setLogs] = useState([]);


  const logEl = useRef(null);

  const controlScroll = () => {
    logEl.current.scrollTop = logEl.current.scrollHeight;
  }

  useEffect(controlScroll, [logs]);

  // setInterval(() => {
  //   setLogs([...logs, "send auto scroll down"]);
  // }, 100)

  let _timer = null;

  useEffect(() => {
    console.log(`login >>>>  ${login}`);
    if (!rtm._logined) {
      rtm.init(APP_ID);
      rtm.on("ConnectionStateChanged", (state, reason) => {
        setRtmConnectState(state);
        toast(`state: ${state}, reason: ${reason}`, {autoClose: true});
        console.log("state ", state, " reason ", reason);
      });
  
      rtm.on("MessageFromPeer", (msg, peerId) => {
        if (peerId === "agora.pi") {
          const decodeText = msg.text.split(",");
          const [battery, connected] = decodeText;
          setBattery(battery);
          setConnect(connected);
          console.log("battery ", battery, " connected ", connected);
        }
        console.log("text ", msg.text, " peerId ", peerId);
      });
      rtm.on("ChannelMessage", (args) => {
        console.log("join channel", JSON.stringify(args));
      })
    }
    if (login === false) {
      console.log(`login >>>>  ${login}`);
      rtm.login("agora.pi-client2").then(() => {
        rtm.joinChannel("agora.pi-server2").then(() => {
          rtm._logined = true;
          setLogin(true);
          console.log("login success", rtm._logined);
          toast("RTM Login success", {autoClose: true});
        }).catch((err) => {
          console.log("join channel failed", JSON.stringify(err));
        })
        // _timer = setInterval(() => {
        //   rtm._logined && rtm.sendPeerMessage("pi,get_info");
        // }, 1500);
      }).catch((err) => {
        toast("RTM Login failure", {autoClose: true});
      })
    }
    // return () => {
    //   _timer && clearInterval(_timer);
    //   _timer = null;
    // }
  }, []);

  let timer = null;

  useEffect(() => {
    if (touch) {
      timer = setInterval(() => {
        rtm.sendPeerMessage(`${direct},${speed}`).then(() => {
          setLogs([...logs, `${direct},${speed}`]);
        })
      }, 300);
    } else {
      timer && clearInterval(timer);
    }
    
    return (() => {
      timer && clearInterval(timer);
    })
  }, [speed, direct, touch]);

  const handleTouch = (method) => {
    return (evt) => {
      const methods = {
        up () {
          setDirect(`up`);
        },
        right () {
          setDirect(`right`);
        },
        down () {
          setDirect(`down`);
        },
        left () {
          setDirect(`left`);
        },
        // start () {
        //   // console.log(`start ${direct},${speed}`)
        //   setStart(true);
        //   console.log("direct", direct, "rtm", rtm);
        //   // setSpeed(Math.min(speed+35, MAX_SPEED));
        //   // setSpeed(50);
        //   setLogs([...logs, `trigger start ${direct},50`]);
        //   // rtm.sendPeerMessage(`${direct},50`).then(() => {
        //   //   setLogs([...logs, `send ${direct},50`]);
        //   // });
        //   setSpeed(20);
        // },
        // stop () {
        //   // console.log("stop")
        //   setStart(false);
        //   // setLogs([...logs, `trigger stop ${direct},50`]);
        //   rtm.sendPeerMessage(`stop,0`).then(() => {
        //     setLogs([...logs, `stop ${direct},50`]);
        //   });
        //   // rtm.sendPeerMessage(`stop,50`);
        //   // setSpeed(Math.max(speed-35, MIN_SPEED));
        //   setSpeed(0);
        // }
      }
      setTouch(true);
      setLogs([...logs, `touch`]);
      methods[method](evt);
    }
  }

  const handleTouchEnd = () => {
    setTouch(false);
    rtm.sendPeerMessage(`stop,${speed}`).then(() => {
      setLogs([...logs, `stop ${direct},${speed}`]);
    });
    setLogs([...logs, `untouch`]);
  }

  const handleSpeedTouch = (method) => {
    return (evt) => {
      const methods = {
        'start': () => {
          setSpeed(Math.min(speed+15, 95));
          setStart(true);
        },
        'stop': () => {
          setSpeed(Math.max(speed-15, 0));
          setStart(false);
          rtm.sendPeerMessage(`stop,0`).then(() => {
            setLogs([...logs, `stop ${direct},50`]);
          });
        }
      }
      setSpeedTouch(true);
      setLogs([...logs, `touch`]);
      methods[method](evt);
    }
  }

  const handleTouchEndSpeed = () => {
    setSpeedTouch(false);
    setStart(null);
  }

  // useEffect(() => {
  //   if (rtm._logined === true) {
  //     if (_timer === null && direct && speed) {
  //       _timer = setInterval(() => {
  //         rtm.sendPeerMessage(`${direct},${speed}`);
  //       }, 800);
  //     }
  //   }

  //   return () => {
  //     if (_timer) {
  //       clearInterval(_timer);
  //       _timer = null;
  //     }
  //   }
  // }, [rtm, touch, speed, direct]);

  return (
    <div className="game-pad">
      <div className="status-bar">
        <div className="logo"></div>
        <div className="info">
          {/* <div className={`icon battery-${battery}`}></div> */}
          {/* <div className={`icon ${connect == 'true' ? 'connect' : 'disconnect'}`}></div> */}
        </div>
      </div>
      <div className="control-bar">
        <div className="direction-control">
          <div className={`btn arrow up ${touch && direct == 'up' ? 'active' : ''}`}
            onTouchStart={handleTouch("up")} onTouchEnd={handleTouchEnd} />
          <div className={`btn arrow right ${touch && direct == 'right' ? 'active' : ''}`}
            onTouchStart={handleTouch("right")} onTouchEnd={handleTouchEnd} />
          <div className={`btn arrow down ${touch && direct == 'down' ? 'active' : ''}`}
            onTouchStart={handleTouch("down")} onTouchEnd={handleTouchEnd} />
          <div className={`btn arrow left ${touch && direct == 'left' ? 'active' : ''}`}
            onTouchStart={handleTouch("left")} onTouchEnd={handleTouchEnd} />
        </div>
        <div ref={logEl} className="log-bar">
          <h5>speed: ${speed}</h5>
          {logs.map((e, i) => (
            <span key={i} className="line">
              {e}
            </span>
          ))}
        </div>
        <div className="speed-control">
          <div className={`btn increment ${speedTouch && start === true ? 'active' : ''}`} onTouchStart={handleSpeedTouch("start")} onTouchEnd={handleTouchEndSpeed}></div>
          <div className={`btn decrement ${speedTouch && start === false ? 'active' : ''}`} onTouchStart={handleSpeedTouch("stop")} onTouchEnd={handleTouchEndSpeed}></div>
        </div>
      </div>
    </div>
  );
}