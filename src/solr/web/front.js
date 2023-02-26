var board_el = document.getElementById("board");
var stacks = [{}];
var freeCards = [];
var gamedata = undefined;

var stackPos = [
  {x: 12,                 y: 64,     },
  {x: 12 + 1 * (12 + 88), y: 140 + 64},
  {x: 12 + 2 * (12 + 88), y: 140 + 64},
  {x: 12 + 3 * (12 + 88), y: 140 + 64},
  {x: 12 + 4 * (12 + 88), y: 140 + 64},
  {x: 12 + 5 * (12 + 88), y: 140 + 64},
  {x: 12 + 6 * (12 + 88), y: 140 + 64},
  {x: 12 + 7 * (12 + 88), y: 140 + 64},
  {x: 12 + 4 * (12 + 88), y: 64,     },
  {x: 12 + 5 * (12 + 88), y: 64,     },
  {x: 12 + 6 * (12 + 88), y: 64,     },
  {x: 12 + 7 * (12 + 88), y: 64,     },
  {x: 12 + 1 * (12 + 88), y: 64,     },
];

const Suit = {
  H: 1,
  D: 2,
  C: 3,
  S: 4 
};

let ATLASES = [];
ATLASES[0]      = "cards/Card_Back-88x124.png"
ATLASES[Suit.H] = "cards/Hearts-88x124.png";
ATLASES[Suit.D] = "cards/Diamonds-88x124.png";
ATLASES[Suit.C] = "cards/Clubs-88x124.png";
ATLASES[Suit.S] = "cards/Spades-88x124.png";
ATLASES[10]     = "cards/Card_Slot-88x124.png";

class Card {
  elem = undefined;
  faceup = true;
  suit = -1;
  rank = -1;

  updateAtlas() {
    let cw = 88; let ch = 124;
    this.elem.style.width  = cw + 'px';
    this.elem.style.height = ch + 'px';
    let atlas = ATLASES[0];
    let x = 0; let y = 0;
    if (this.faceup == true) {
      atlas = ATLASES[this.suit];
      x = (this.rank % 5) * cw; y = Math.floor(this.rank / 5) * ch;
    }
    this.elem.style.backgroundImage = 'url(' + atlas + ')';
    this.elem.style.backgroundPosition = '-' + x + 'px -' + y + 'px';
  }

  updatePos(x,y) {
    this.elem.style.position = 'absolute';
    this.elem.style.left = x + 'px';
    this.elem.style.top  = y + 'px';
    this.elem.style.display = 'block';
  }

  free() {
    this.elem.style.display = 'none';
    this.faceup = false;
  }
}

for (let stack = 0; stack < 13; stack++) {
  let stackarr = [];
  let stackel = document.createElement("div");
  stackel.style.position = "absolute";
  stackel.style.left = stackPos[stack].x + 'px';
  stackel.style.top  = stackPos[stack].y + 'px';
  let img = document.createElement("img");
  img.src = ATLASES[10];
  document.body.appendChild(stackel);
  stackel.appendChild(img);
  stacks[stack] = {elem: stackel, cards: stackarr};
}

for (let s = Suit.H; s <= Suit.S; s++) {
  for (let r = 0; r < 13; r++) {
    let cardel = document.createElement("div");
    let c = new Card();
    let pos = stackPos[0];
    c.faceup = false;
    c.elem = cardel;
    c.updatePos(pos);
    c.free();

    document.body.appendChild(cardel);
    freeCards.push(c);
  }
}

function presentState(s) {
  for (stack of s.stacks) {
    let pos = stackPos[stack.id];
    for (card of stack.cards) {
      if (card.up == 0) {
        let selc = nextUnturned();
        selc.updatePos(pos.x, pos.y);
      }
      else {
        let selc = nextUnturned();
        selc.suit = card.s;
        selc.rank = card.r;
        selc.faceup = true;
        selc.updateAtlas();
        selc.updatePos(pos.x, pos.y);
      }
    }
  }
}

(function getState() {
  fetch("game.json")
    .then((response) => { return response.json(); })
    .then((data) => { gamedata = data; presentState(data); });
  setTimeout(getState, 1000);
})();

function findCard(cards, s, r) {
  return cards[s][r];
}

function nextUnturned() {
  let card = nextInStackUnturned(freeCards);
  if (card) { return card; }
  for (stack of stacks) {
    let card = nextInStackUnturned(stack.cards);
    if (card) { return card; }
  }
}

function nextInStackUnturned(sarr) {
  return sarr.find(x => x.faceup === false);
}

