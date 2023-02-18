var board_el = document.getElementById("board");
var cards = [[]];

const Suit = {
  H: 1,
  D: 2,
  C: 3,
  S: 4 
};

let ATLASES = [];
ATLASES[0]      = "cards/Card-Back-88x124.png"
ATLASES[Suit.H] = "cards/Hearts-88x124.png";
ATLASES[Suit.D] = "cards/Diamonds-88x124.png";
ATLASES[Suit.C] = "cards/Clubs-88x124.png";
ATLASES[Suit.S] = "cards/Spades-88x124.png";

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
    this.elem.style.backgroundPosition = '-' + x + 'px -' + y + 'px';  }
}

for (let s = Suit.H; s <= Suit.S; s++) {
  let suitarr = [];
  for (let r = 0; r < 13; r++) {
    let cardel = document.createElement("div");
    let c = new Card();
    c.suit = s; c.rank = r; c.faceup = true;
    c.elem = cardel;
    c.updateAtlas();
    document.body.appendChild(cardel);
    suitarr.push(c);
  }
  cards.push(suitarr);
}

