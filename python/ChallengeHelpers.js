function decodeChallenge(challengeText) {
    const matcher = /'(\d*[a-z]*[A-Z]*)\w+'/,
        equals = challengeText.search('='),
        right_side = challengeText.slice(equals + 1),
        semi = right_side.search(';'),
        offsetEq = right_side.slice(0, Math.max(0, semi)).trim(),
        message = matcher.exec(challengeText);
    let message2 = (message && message.length > 0 ? message[0] : '').slice(1, -1)
    let offset = eval?.(`"use strict";(${offsetEq})`)
    return {
        msg: message2,
        off: offset
    }
}

let result = decodeChallenge(process.argv[2])
console.log(JSON.stringify(result))