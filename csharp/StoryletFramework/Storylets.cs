/*
 * This file is part of an MIT-licensed project: see LICENSE file or README.md for details.
 * Copyright (c) 2025 Ian Thomas
 */

using ExpressionParser;

namespace StoryletFramework;

using Context = Dictionary<string, object>;
using KeyMap = Dictionary<string, object>;

public class Storylet
{
    private static readonly Parser expressionParser = new Parser();

    public string Id { get; private set; }
    public dynamic? Content { get; set; }

    // Redraw settings
    public const int REDRAW_ALWAYS = 0;
    public const int REDRAW_NEVER = -1;
    public int Redraw { get; set; } = REDRAW_ALWAYS;

    // Updates to context when played
    public KeyMap? Outcomes { get; set; }

    // Precompiled condition and priority
    private ExpressionNode? _condition;
    private object _priority = 0;

    // The next draw this should be available
    private int _nextPlay = 0;

    internal Deck? deck = null;

    public Storylet(string id)
    {
        Id = id;
    }

    // Reset the redraw counter
    public void Reset()
    {
        _nextPlay = 0;
    }

    // Set condition as a precompiled expression
    public void SetCondition(string? text)
    {
        _condition = null;
        if (!string.IsNullOrEmpty(text))
        {
            _condition = expressionParser.Parse(text);
        }
    }

    // Evaluate condition using the current context. Returns true if no condition is set.
    public bool CheckCondition(Context context, List<string>? dumpEval = null)
    {
        if (_condition == null)
            return true;

        dumpEval?.Add($"Evaluating condition for {Id}");
        return Convert.ToBoolean(_condition.Evaluate(context, dumpEval));
    }

    // Set priority to a fixed number or a precompiled expression
    public void SetPriority(object numOrExpression)
    {
        if (numOrExpression is int || numOrExpression is double)
        {
            _priority = numOrExpression;
        }
        else if (numOrExpression is string expression)
        {
            _priority = expressionParser.Parse(expression);
        }
        else
        {
            throw new ArgumentException("Priority must be a number or a valid expression.");
        }
    }

    // Evaluate priority using the current context
    public int CalcCurrentPriority(Context context, bool useSpecificity = true, List<string>? dumpEval = null)
    {
        int workingPriority = 0;

        if (_priority is int || _priority is double)
        {
            workingPriority = Convert.ToInt32(_priority);
        }
        else if (_priority is ExpressionNode expression)
        {
            dumpEval?.Add($"Evaluating priority for {Id}");
            workingPriority = Convert.ToInt32(expression.Evaluate(context, dumpEval));
        }

        if (useSpecificity)
        {
            workingPriority *= 100;
            if (_condition != null)
            {
                workingPriority += _condition.Specificity;
            }
        }

        return workingPriority;
    }

    // Check if the storylet is available to draw based on redraw rules
    public bool CanDraw(int currentPlay)
    {
        if (Redraw == REDRAW_NEVER && _nextPlay < 0)
            return false;
        if (Redraw == REDRAW_ALWAYS)
            return true;
        return currentPlay >= _nextPlay;
    }

    // Update the redraw counter when the storylet is drawn
    internal void OnPlayed(int currentDraw, Context context, string outcome="default", List<string>? dumpEval = null)
    {
        if (dumpEval != null)
        {
            dumpEval.Add($"Storylet {Id} played with outcome '{outcome}' at draw {currentDraw}.");
        }

        if (Redraw == REDRAW_NEVER)
        {
            _nextPlay = -1;
        }
        else
        {
            _nextPlay = currentDraw + Redraw;
        }

        if (Outcomes != null && Outcomes.ContainsKey(outcome))
        {
            var updates = Outcomes[outcome];
            if (dumpEval != null) {
                dumpEval.Add($"Updating context for {Id} with outcome '${outcome}");
            }
            ContextUtils.UpdateContext(context, (KeyMap)updates, dumpEval);
        }
    }

      // Convenience. Play the storylet. Updates draw counters and applies any updateOnPlayed
    public void Play(string outcome = "default", List<string>? dumpEval = null) {
        if (deck == null) {
            throw new Exception("Storylet not in deck.");
        }
        deck.Play(this, outcome, dumpEval);
    }

}

public class Deck
{
    public bool UseSpecificity { get; set; } = false;
    private readonly Dictionary<string, Storylet> _all = new();
    private int _currentPlay = 0;
    public readonly Context Context;

    public Deck(Context? context = null)
    {
        Context = context ?? new Context();
    }

    // Reset the whole deck, including all redraw counters
    public void Reset()
    {
        _currentPlay = 0;
        foreach (var storylet in _all.Values)
        {
            storylet.Reset();
        }
    }

    // Reshuffle and draw from the deck
    public List<Storylet> Draw(int count = -1, Func<Storylet, bool>? filter = null, List<string>? dumpEval = null)
    {
        var priorityMap = new SortedDictionary<int, List<Storylet>>(Comparer<int>.Create((a, b) => b.CompareTo(a)));
        var toProcess = _all.Values.ToList();

        while (toProcess.Count > 0)  
        {
            var storylet = toProcess[0];
            toProcess.RemoveAt(0);

            if (!storylet.CanDraw(_currentPlay))
                continue;

            if (filter != null && !filter(storylet))
                continue;

            if (!storylet.CheckCondition(Context, dumpEval))
                continue;

            var priority = storylet.CalcCurrentPriority(Context, UseSpecificity, dumpEval);

            if (!priorityMap.ContainsKey(priority))
            {
                priorityMap[priority] = new List<Storylet>();
            }

            priorityMap[priority].Add(storylet);
        }

        var drawPile = new List<Storylet>();

        foreach (var bucket in priorityMap.Values)
        {
            Utils.ShuffleArray(bucket);
            drawPile.AddRange(bucket);
            if (count>-1 && drawPile.Count >= count)
            {
                break;
            }
        }

        return count > -1 ? [.. drawPile.Take(count)] : drawPile;
    }

    public List<Storylet> DrawAndPlay(int count = -1, Func<Storylet, bool>? filter = null, string outcome="default", List<string>? dumpEval = null)
    {
        var drawPile = Draw(count, filter, dumpEval);
        foreach (var storylet in drawPile)
        {
            Play(storylet, outcome, dumpEval);
        }
        return drawPile;
    }

    public Storylet? DrawSingle(Func<Storylet, bool>? filter = null, List<string>? dumpEval = null)
    {
        var drawPile = Draw(1, filter, dumpEval);
        if (drawPile.Count > 0)
        {
            return drawPile[0];
        }
        return null;
    }

    public Storylet? DrawAndPlaySingle(Func<Storylet, bool>? filter = null, string outcome="default", List<string>? dumpEval = null)
    {
        var storylet = DrawSingle(filter, dumpEval);
        if (storylet != null)
        {
            Play(storylet, outcome, dumpEval);
        }
        return storylet;
    }

    public Storylet? GetStorylet(string id)
    {
        if (_all.TryGetValue(id, out var storylet))
        {
            return storylet;
        }
        return null;
    }

    public void AddStorylet(Storylet storylet)
    {
        if (_all.ContainsKey(storylet.Id))
            throw new ArgumentException($"Duplicate storylet id: '{storylet.Id}'.");

        _all[storylet.Id] = storylet;
        storylet.deck = this;
    }

    public void Play(Storylet storylet, string outcome = "default", List<string>? dumpEval = null) {
        _currentPlay++;
        storylet.OnPlayed(_currentPlay, Context, outcome, dumpEval);
    }
}
